// SPDX-License-Identifier: GPL-2.0
/*
 * NTP state machine interfaces and logic.
 *
 * This code was mainly moved from kernel/timer.c and kernel/time.c
 * Please see those files for relevant copyright info and historical
 * changelogs.
 */
#include <linux/capability.h>
#include <linux/clocksource.h>
#include <linux/workqueue.h>
#include <linux/hrtimer.h>
#include <linux/jiffies.h>
#include <linux/math64.h>
#include <linux/timex.h>
#include <linux/time.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/rtc.h>
#include <linux/audit.h>
#include <linux/timekeeper_internal.h>

#include "ntp_internal.h"
#include "timekeeping_internal.h"

/**
 * struct ntp_data - Structure holding all NTP related state
 * @tick_usec:		USER_HZ period in microseconds
 * @tick_length:	Adjusted tick length
 * @tick_length_base:	Base value for @tick_length
 * @time_state:		State of the clock synchronization
 * @time_status:	Clock status bits
 * @time_offset:	Time adjustment in nanoseconds
 * @time_constant:	PLL time constant
 * @time_maxerror:	Maximum error in microseconds holding the NTP sync distance
 *			(NTP dispersion + delay / 2)
 * @time_esterror:	Estimated error in microseconds holding NTP dispersion
 * @time_freq:		Frequency offset scaled nsecs/secs
 * @time_reftime:	Time at last adjustment in seconds
 * @time_adjust:	Adjustment value
 * @ntp_tick_adj:	Constant boot-param configurable NTP tick adjustment (upscaled)
 * @ntp_next_leap_sec:	Second value of the next pending leapsecond, or TIME64_MAX if no leap
 *
 * @pps_valid:		PPS signal watchdog counter
 * @pps_tf:		PPS phase median filter
 * @pps_jitter:		PPS current jitter in nanoseconds
 * @pps_fbase:		PPS beginning of the last freq interval
 * @pps_shift:		PPS current interval duration in seconds (shift value)
 * @pps_intcnt:		PPS interval counter
 * @pps_freq:		PPS frequency offset in scaled ns/s
 * @pps_stabil:		PPS current stability in scaled ns/s
 * @pps_calcnt:		PPS monitor: calibration intervals
 * @pps_jitcnt:		PPS monitor: jitter limit exceeded
 * @pps_stbcnt:		PPS monitor: stability limit exceeded
 * @pps_errcnt:		PPS monitor: calibration errors
 *
 * Protected by the timekeeping locks.
 */
struct ntp_data {
	unsigned long		tick_usec;
	u64			tick_length;
	u64			tick_length_base;
	int			time_state;
	int			time_status;
	s64			time_offset;
	long			time_constant;
	long			time_maxerror;
	long			time_esterror;
	s64			time_freq;
	time64_t		time_reftime;
	long			time_adjust;
	s64			ntp_tick_adj;
	time64_t		ntp_next_leap_sec;
};

static struct ntp_data tk_ntp_data[TIMEKEEPERS_MAX] = {
	[ 0 ... TIMEKEEPERS_MAX - 1 ] = {
		.tick_usec		= USER_TICK_USEC,
		.time_state		= TIME_OK,
		.time_status		= STA_UNSYNC,
		.time_constant		= 2,
		.time_maxerror		= NTP_PHASE_LIMIT,
		.time_esterror		= NTP_PHASE_LIMIT,
		.ntp_next_leap_sec	= TIME64_MAX,
	},
};

#define SECS_PER_DAY		86400
#define MAX_TICKADJ		500LL		/* usecs */
#define MAX_TICKADJ_SCALED \
	(((MAX_TICKADJ * NSEC_PER_USEC) << NTP_SCALE_SHIFT) / NTP_INTERVAL_FREQ)
#define MAX_TAI_OFFSET		100000


static inline s64 ntp_offset_chunk(struct ntp_data *ntpdata, s64 offset)
{
	return shift_right(offset, SHIFT_PLL + ntpdata->time_constant);
}

static inline void pps_reset_freq_interval(struct ntp_data *ntpdata) {}
static inline void pps_clear(struct ntp_data *ntpdata) {}
static inline void pps_dec_valid(struct ntp_data *ntpdata) {}
static inline void pps_set_freq(struct ntp_data *ntpdata) {}

static inline bool is_error_status(int status)
{
	return status & (STA_UNSYNC|STA_CLOCKERR);
}

static inline void pps_fill_timex(struct ntp_data *ntpdata, struct __kernel_timex *txc)
{
	/* PPS is not implemented, so these are zero */
	txc->ppsfreq	   = 0;
	txc->jitter	   = 0;
	txc->shift	   = 0;
	txc->stabil	   = 0;
	txc->jitcnt	   = 0;
	txc->calcnt	   = 0;
	txc->errcnt	   = 0;
	txc->stbcnt	   = 0;
}


/*
 * Update tick_length and tick_length_base, based on tick_usec, ntp_tick_adj and
 * time_freq:
 */
static void ntp_update_frequency(struct ntp_data *ntpdata)
{
	u64 second_length, new_base, tick_usec = (u64)ntpdata->tick_usec;

	second_length		 = (u64)(tick_usec * NSEC_PER_USEC * USER_HZ) << NTP_SCALE_SHIFT;

	second_length		+= ntpdata->ntp_tick_adj;
	second_length		+= ntpdata->time_freq;

	new_base		 = div_u64(second_length, NTP_INTERVAL_FREQ);

	/*
	 * Don't wait for the next second_overflow, apply the change to the
	 * tick length immediately:
	 */
	ntpdata->tick_length		+= new_base - ntpdata->tick_length_base;
	ntpdata->tick_length_base	 = new_base;
}

static inline s64 ntp_update_offset_fll(struct ntp_data *ntpdata, s64 offset64, long secs)
{
	ntpdata->time_status &= ~STA_MODE;

	if (secs < MINSEC)
		return 0;

	if (!(ntpdata->time_status & STA_FLL) && (secs <= MAXSEC))
		return 0;

	ntpdata->time_status |= STA_MODE;

	return div64_long(offset64 << (NTP_SCALE_SHIFT - SHIFT_FLL), secs);
}

static void ntp_update_offset(struct ntp_data *ntpdata, long offset)
{
	s64 freq_adj, offset64;
	long secs, real_secs;

	if (!(ntpdata->time_status & STA_PLL))
		return;

	if (!(ntpdata->time_status & STA_NANO)) {
		/* Make sure the multiplication below won't overflow */
		offset = clamp(offset, -USEC_PER_SEC, USEC_PER_SEC);
		offset *= NSEC_PER_USEC;
	}

	/* Scale the phase adjustment and clamp to the operating range. */
	offset = clamp(offset, -MAXPHASE, MAXPHASE);

	/*
	 * Select how the frequency is to be controlled
	 * and in which mode (PLL or FLL).
	 */
	real_secs = ktime_get_ntp_seconds(ntpdata - tk_ntp_data);
	secs = (long)(real_secs - ntpdata->time_reftime);
	if (unlikely(ntpdata->time_status & STA_FREQHOLD))
		secs = 0;

	ntpdata->time_reftime = real_secs;

	offset64    = offset;
	freq_adj    = ntp_update_offset_fll(ntpdata, offset64, secs);

	/*
	 * Clamp update interval to reduce PLL gain with low
	 * sampling rate (e.g. intermittent network connection)
	 * to avoid instability.
	 */
	if (unlikely(secs > 1 << (SHIFT_PLL + 1 + ntpdata->time_constant)))
		secs = 1 << (SHIFT_PLL + 1 + ntpdata->time_constant);

	freq_adj    += (offset64 * secs) <<
			(NTP_SCALE_SHIFT - 2 * (SHIFT_PLL + 2 + ntpdata->time_constant));

	freq_adj    = min(freq_adj + ntpdata->time_freq, MAXFREQ_SCALED);

	ntpdata->time_freq   = max(freq_adj, -MAXFREQ_SCALED);

	ntpdata->time_offset = div_s64(offset64 << NTP_SCALE_SHIFT, NTP_INTERVAL_FREQ);
}

static void __ntp_clear(struct ntp_data *ntpdata)
{
	/* Stop active adjtime() */
	ntpdata->time_adjust	= 0;
	ntpdata->time_status	|= STA_UNSYNC;
	ntpdata->time_maxerror	= NTP_PHASE_LIMIT;
	ntpdata->time_esterror	= NTP_PHASE_LIMIT;

	ntp_update_frequency(ntpdata);

	ntpdata->tick_length	= ntpdata->tick_length_base;
	ntpdata->time_offset	= 0;

	ntpdata->ntp_next_leap_sec = TIME64_MAX;
	/* Clear PPS state variables */
	pps_clear(ntpdata);
}

/**
 * ntp_clear - Clears the NTP state variables
 * @tkid:	Timekeeper ID to be able to select proper ntp data array member
 */
void ntp_clear(unsigned int tkid)
{
	__ntp_clear(&tk_ntp_data[tkid]);
}


u64 ntp_tick_length(unsigned int tkid)
{
	return tk_ntp_data[tkid].tick_length;
}

/**
 * ntp_get_next_leap - Returns the next leapsecond in CLOCK_REALTIME ktime_t
 * @tkid:	Timekeeper ID
 *
 * Returns: For @tkid == TIMEKEEPER_CORE this provides the time of the next
 *	    leap second against CLOCK_REALTIME in a ktime_t format if a
 *	    leap second is pending. KTIME_MAX otherwise.
 */
ktime_t ntp_get_next_leap(unsigned int tkid)
{
	struct ntp_data *ntpdata = &tk_ntp_data[TIMEKEEPER_CORE];

	if (tkid != TIMEKEEPER_CORE)
		return KTIME_MAX;

	if ((ntpdata->time_state == TIME_INS) && (ntpdata->time_status & STA_INS))
		return ktime_set(ntpdata->ntp_next_leap_sec, 0);

	return KTIME_MAX;
}

/*
 * This routine handles the overflow of the microsecond field
 *
 * The tricky bits of code to handle the accurate clock support
 * were provided by Dave Mills (Mills@UDEL.EDU) of NTP fame.
 * They were originally developed for SUN and DEC kernels.
 * All the kudos should go to Dave for this stuff.
 *
 * Also handles leap second processing, and returns leap offset
 */
int second_overflow(unsigned int tkid, time64_t secs)
{
	struct ntp_data *ntpdata = &tk_ntp_data[tkid];
	s64 delta;
	int leap = 0;
	s32 rem;

	/*
	 * Leap second processing. If in leap-insert state at the end of the
	 * day, the system clock is set back one second; if in leap-delete
	 * state, the system clock is set ahead one second.
	 */
	switch (ntpdata->time_state) {
	case TIME_OK:
		if (ntpdata->time_status & STA_INS) {
			ntpdata->time_state = TIME_INS;
			div_s64_rem(secs, SECS_PER_DAY, &rem);
			ntpdata->ntp_next_leap_sec = secs + SECS_PER_DAY - rem;
		} else if (ntpdata->time_status & STA_DEL) {
			ntpdata->time_state = TIME_DEL;
			div_s64_rem(secs + 1, SECS_PER_DAY, &rem);
			ntpdata->ntp_next_leap_sec = secs + SECS_PER_DAY - rem;
		}
		break;
	case TIME_INS:
		if (!(ntpdata->time_status & STA_INS)) {
			ntpdata->ntp_next_leap_sec = TIME64_MAX;
			ntpdata->time_state = TIME_OK;
		} else if (secs == ntpdata->ntp_next_leap_sec) {
			leap = -1;
			ntpdata->time_state = TIME_OOP;
			pr_notice("Clock: inserting leap second 23:59:60 UTC\n");
		}
		break;
	case TIME_DEL:
		if (!(ntpdata->time_status & STA_DEL)) {
			ntpdata->ntp_next_leap_sec = TIME64_MAX;
			ntpdata->time_state = TIME_OK;
		} else if (secs == ntpdata->ntp_next_leap_sec) {
			leap = 1;
			ntpdata->ntp_next_leap_sec = TIME64_MAX;
			ntpdata->time_state = TIME_WAIT;
			pr_notice("Clock: deleting leap second 23:59:59 UTC\n");
		}
		break;
	case TIME_OOP:
		ntpdata->ntp_next_leap_sec = TIME64_MAX;
		ntpdata->time_state = TIME_WAIT;
		break;
	case TIME_WAIT:
		if (!(ntpdata->time_status & (STA_INS | STA_DEL)))
			ntpdata->time_state = TIME_OK;
		break;
	}

	/* Bump the maxerror field */
	ntpdata->time_maxerror += MAXFREQ / NSEC_PER_USEC;
	if (ntpdata->time_maxerror > NTP_PHASE_LIMIT) {
		ntpdata->time_maxerror = NTP_PHASE_LIMIT;
		ntpdata->time_status |= STA_UNSYNC;
	}

	/* Compute the phase adjustment for the next second */
	ntpdata->tick_length	 = ntpdata->tick_length_base;

	delta			 = ntp_offset_chunk(ntpdata, ntpdata->time_offset);
	ntpdata->time_offset	-= delta;
	ntpdata->tick_length	+= delta;

	/* Check PPS signal */
	pps_dec_valid(ntpdata);

	if (!ntpdata->time_adjust)
		goto out;

	if (ntpdata->time_adjust > MAX_TICKADJ) {
		ntpdata->time_adjust -= MAX_TICKADJ;
		ntpdata->tick_length += MAX_TICKADJ_SCALED;
		goto out;
	}

	if (ntpdata->time_adjust < -MAX_TICKADJ) {
		ntpdata->time_adjust += MAX_TICKADJ;
		ntpdata->tick_length -= MAX_TICKADJ_SCALED;
		goto out;
	}

	ntpdata->tick_length += (s64)(ntpdata->time_adjust * NSEC_PER_USEC / NTP_INTERVAL_FREQ)
				<< NTP_SCALE_SHIFT;
	ntpdata->time_adjust = 0;

out:
	return leap;
}

static inline void __init ntp_init_cmos_sync(void) { }

/*
 * Propagate a new txc->status value into the NTP state:
 */
static inline void process_adj_status(struct ntp_data *ntpdata, const struct __kernel_timex *txc)
{
	if ((ntpdata->time_status & STA_PLL) && !(txc->status & STA_PLL)) {
		ntpdata->time_state = TIME_OK;
		ntpdata->time_status = STA_UNSYNC;
		ntpdata->ntp_next_leap_sec = TIME64_MAX;
		/* Restart PPS frequency calibration */
		pps_reset_freq_interval(ntpdata);
	}

	/*
	 * If we turn on PLL adjustments then reset the
	 * reference time to current time.
	 */
	if (!(ntpdata->time_status & STA_PLL) && (txc->status & STA_PLL))
		ntpdata->time_reftime = ktime_get_ntp_seconds(ntpdata - tk_ntp_data);

	/* only set allowed bits */
	ntpdata->time_status &= STA_RONLY;
	ntpdata->time_status |= txc->status & ~STA_RONLY;
}

static inline void process_adjtimex_modes(struct ntp_data *ntpdata, const struct __kernel_timex *txc,
					  s32 *time_tai)
{
	if (txc->modes & ADJ_STATUS)
		process_adj_status(ntpdata, txc);

	if (txc->modes & ADJ_NANO)
		ntpdata->time_status |= STA_NANO;

	if (txc->modes & ADJ_MICRO)
		ntpdata->time_status &= ~STA_NANO;

	if (txc->modes & ADJ_FREQUENCY) {
		ntpdata->time_freq = txc->freq * PPM_SCALE;
		ntpdata->time_freq = min(ntpdata->time_freq, MAXFREQ_SCALED);
		ntpdata->time_freq = max(ntpdata->time_freq, -MAXFREQ_SCALED);
		/* Update pps_freq */
		pps_set_freq(ntpdata);
	}

	if (txc->modes & ADJ_MAXERROR)
		ntpdata->time_maxerror = clamp(txc->maxerror, 0, NTP_PHASE_LIMIT);

	if (txc->modes & ADJ_ESTERROR)
		ntpdata->time_esterror = clamp(txc->esterror, 0, NTP_PHASE_LIMIT);

	if (txc->modes & ADJ_TIMECONST) {
		ntpdata->time_constant = clamp(txc->constant, 0, MAXTC);
		if (!(ntpdata->time_status & STA_NANO))
			ntpdata->time_constant += 4;
		ntpdata->time_constant = clamp(ntpdata->time_constant, 0, MAXTC);
	}

	if (txc->modes & ADJ_TAI && txc->constant >= 0 && txc->constant <= MAX_TAI_OFFSET)
		*time_tai = txc->constant;

	if (txc->modes & ADJ_OFFSET)
		ntp_update_offset(ntpdata, txc->offset);

	if (txc->modes & ADJ_TICK)
		ntpdata->tick_usec = txc->tick;

	if (txc->modes & (ADJ_TICK|ADJ_FREQUENCY|ADJ_OFFSET))
		ntp_update_frequency(ntpdata);
}

/*
 * adjtimex() mainly allows reading (and writing, if superuser) of
 * kernel time-keeping variables. used by xntpd.
 */
int ntp_adjtimex(unsigned int tkid, struct __kernel_timex *txc, const struct timespec64 *ts,
		 s32 *time_tai, struct audit_ntp_data *ad)
{
	struct ntp_data *ntpdata = &tk_ntp_data[tkid];
	int result;

	if (txc->modes & ADJ_ADJTIME) {
		long save_adjust = ntpdata->time_adjust;

		if (!(txc->modes & ADJ_OFFSET_READONLY)) {
			/* adjtime() is independent from ntp_adjtime() */
			ntpdata->time_adjust = txc->offset;
			ntp_update_frequency(ntpdata);

			audit_ntp_set_old(ad, AUDIT_NTP_ADJUST,	save_adjust);
			audit_ntp_set_new(ad, AUDIT_NTP_ADJUST,	ntpdata->time_adjust);
		}
		txc->offset = save_adjust;
	} else {
		/* If there are input parameters, then process them: */
		if (txc->modes) {
			audit_ntp_set_old(ad, AUDIT_NTP_OFFSET,	ntpdata->time_offset);
			audit_ntp_set_old(ad, AUDIT_NTP_FREQ,	ntpdata->time_freq);
			audit_ntp_set_old(ad, AUDIT_NTP_STATUS,	ntpdata->time_status);
			audit_ntp_set_old(ad, AUDIT_NTP_TAI,	*time_tai);
			audit_ntp_set_old(ad, AUDIT_NTP_TICK,	ntpdata->tick_usec);

			process_adjtimex_modes(ntpdata, txc, time_tai);

			audit_ntp_set_new(ad, AUDIT_NTP_OFFSET,	ntpdata->time_offset);
			audit_ntp_set_new(ad, AUDIT_NTP_FREQ,	ntpdata->time_freq);
			audit_ntp_set_new(ad, AUDIT_NTP_STATUS,	ntpdata->time_status);
			audit_ntp_set_new(ad, AUDIT_NTP_TAI,	*time_tai);
			audit_ntp_set_new(ad, AUDIT_NTP_TICK,	ntpdata->tick_usec);
		}

		txc->offset = shift_right(ntpdata->time_offset * NTP_INTERVAL_FREQ, NTP_SCALE_SHIFT);
		if (!(ntpdata->time_status & STA_NANO))
			txc->offset = div_s64(txc->offset, NSEC_PER_USEC);
	}

	result = ntpdata->time_state;
	if (is_error_status(ntpdata->time_status))
		result = TIME_ERROR;

	txc->freq	   = shift_right((ntpdata->time_freq >> PPM_SCALE_INV_SHIFT) *
					 PPM_SCALE_INV, NTP_SCALE_SHIFT);
	txc->maxerror	   = ntpdata->time_maxerror;
	txc->esterror	   = ntpdata->time_esterror;
	txc->status	   = ntpdata->time_status;
	txc->constant	   = ntpdata->time_constant;
	txc->precision	   = 1;
	txc->tolerance	   = MAXFREQ_SCALED / PPM_SCALE;
	txc->tick	   = ntpdata->tick_usec;
	txc->tai	   = *time_tai;

	/* Fill PPS status fields */
	pps_fill_timex(ntpdata, txc);

	txc->time.tv_sec = ts->tv_sec;
	txc->time.tv_usec = ts->tv_nsec;
	if (!(ntpdata->time_status & STA_NANO))
		txc->time.tv_usec = ts->tv_nsec / NSEC_PER_USEC;

	/* Handle leapsec adjustments */
	if (unlikely(ts->tv_sec >= ntpdata->ntp_next_leap_sec)) {
		if ((ntpdata->time_state == TIME_INS) && (ntpdata->time_status & STA_INS)) {
			result = TIME_OOP;
			txc->tai++;
			txc->time.tv_sec--;
		}
		if ((ntpdata->time_state == TIME_DEL) && (ntpdata->time_status & STA_DEL)) {
			result = TIME_WAIT;
			txc->tai--;
			txc->time.tv_sec++;
		}
		if ((ntpdata->time_state == TIME_OOP) && (ts->tv_sec == ntpdata->ntp_next_leap_sec))
			result = TIME_WAIT;
	}

	return result;
}


static int __init ntp_tick_adj_setup(char *str)
{
	int rc = kstrtos64(str, 0, &tk_ntp_data[TIMEKEEPER_CORE].ntp_tick_adj);
	if (rc)
		return rc;

	tk_ntp_data[TIMEKEEPER_CORE].ntp_tick_adj <<= NTP_SCALE_SHIFT;
	return 1;
}
__setup("ntp_tick_adj=", ntp_tick_adj_setup);

void __init ntp_init(void)
{
	for (int id = 0; id < TIMEKEEPERS_MAX; id++)
		__ntp_clear(tk_ntp_data + id);
	ntp_init_cmos_sync();
}
