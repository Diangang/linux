/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _TIMEKEEPING_INTERNAL_H
#define _TIMEKEEPING_INTERNAL_H

#include <linux/clocksource.h>
#include <linux/spinlock.h>
#include <linux/time.h>

/*
 * timekeeping debug functions
 */

#define tk_debug_account_sleep_time(x)

static inline void timekeeping_inc_mg_floor_swaps(void)
{
}


static inline u64 clocksource_delta(u64 now, u64 last, u64 mask, u64 max_delta)
{
	u64 ret = (now - last) & mask;

	/*
	 * Prevent time going backwards by checking the result against
	 * @max_delta. If greater, return 0.
	 */
	return ret > max_delta ? 0 : ret;
}

/* Semi public for serialization of non timekeeper VDSO updates. */
unsigned long timekeeper_lock_irqsave(void);
void timekeeper_unlock_irqrestore(unsigned long flags);

/* NTP specific interface to access the current seconds value */
long ktime_get_ntp_seconds(unsigned int id);

#endif /* _TIMEKEEPING_INTERNAL_H */
