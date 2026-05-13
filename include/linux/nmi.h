/* SPDX-License-Identifier: GPL-2.0 */
/*
 *  linux/include/linux/nmi.h
 */
#ifndef LINUX_NMI_H
#define LINUX_NMI_H

#include <linux/sched.h>
#include <asm/irq.h>

/* Arch specific watchdogs might need to share extra watchdog-related APIs. */
#if defined(CONFIG_HARDLOCKUP_DETECTOR_ARCH) || defined(CONFIG_HARDLOCKUP_DETECTOR_SPARC64)
#include <asm/nmi.h>
#endif

static inline void lockup_detector_init(void) { }

static inline void touch_softlockup_watchdog_sched(void) { }
static inline void touch_softlockup_watchdog(void) { }
static inline void touch_softlockup_watchdog_sync(void) { }
static inline void touch_all_softlockup_watchdogs(void) { }

static inline void reset_hung_task_detector(void) { }

static inline void hardlockup_detector_disable(void) {}

/* Sparc64 has special implemetantion that is always enabled. */
#if defined(CONFIG_HARDLOCKUP_DETECTOR) || defined(CONFIG_HARDLOCKUP_DETECTOR_SPARC64)
void arch_touch_nmi_watchdog(void);
#else
static inline void arch_touch_nmi_watchdog(void) { }
#endif

#if 0
void watchdog_hardlockup_touch_cpu(unsigned int cpu);
void watchdog_hardlockup_check(unsigned int cpu, struct pt_regs *regs);
#endif

static inline void hardlockup_detector_perf_stop(void) { }
static inline void hardlockup_detector_perf_restart(void) { }

static inline void watchdog_buddy_check_hardlockup(int hrtimer_interrupts) {}

/**
 * touch_nmi_watchdog - manually reset the hardlockup watchdog timeout.
 *
 * If we support detecting hardlockups, touch_nmi_watchdog() may be
 * used to pet the watchdog (reset the timeout) - for code which
 * intentionally disables interrupts for a long time. This call is stateless.
 *
 * Though this function has "nmi" in the name, the hardlockup watchdog might
 * not be backed by NMIs. This function will likely be renamed to
 * touch_hardlockup_watchdog() in the future.
 */
static inline void touch_nmi_watchdog(void)
{
	/*
	 * Pass on to the hardlockup detector selected via CONFIG_. Note that
	 * the hardlockup detector may not be arch-specific nor using NMIs
	 * and the arch_touch_nmi_watchdog() function will likely be renamed
	 * in the future.
	 */
	arch_touch_nmi_watchdog();

	touch_softlockup_watchdog();
}

/*
 * Create trigger_all_cpu_backtrace() out of the arch-provided
 * base function. Return whether such support was available,
 * to allow calling code to fall back to some other mechanism:
 */
#ifdef arch_trigger_cpumask_backtrace
static inline bool trigger_all_cpu_backtrace(void)
{
	arch_trigger_cpumask_backtrace(cpu_online_mask, -1);
	return true;
}

static inline bool trigger_allbutcpu_cpu_backtrace(int exclude_cpu)
{
	arch_trigger_cpumask_backtrace(cpu_online_mask, exclude_cpu);
	return true;
}

static inline bool trigger_cpumask_backtrace(struct cpumask *mask)
{
	arch_trigger_cpumask_backtrace(mask, -1);
	return true;
}

static inline bool trigger_single_cpu_backtrace(int cpu)
{
	arch_trigger_cpumask_backtrace(cpumask_of(cpu), -1);
	return true;
}

/* generic implementation */
void nmi_trigger_cpumask_backtrace(const cpumask_t *mask,
				   int exclude_cpu,
				   void (*raise)(cpumask_t *mask));
bool nmi_cpu_backtrace(struct pt_regs *regs);

#else
static inline bool trigger_all_cpu_backtrace(void)
{
	return false;
}
static inline bool trigger_allbutcpu_cpu_backtrace(int exclude_cpu)
{
	return false;
}
static inline bool trigger_cpumask_backtrace(struct cpumask *mask)
{
	return false;
}
static inline bool trigger_single_cpu_backtrace(int cpu)
{
	return false;
}
#endif

#ifdef CONFIG_HAVE_ACPI_APEI_NMI
#include <asm/nmi.h>
#endif

static inline void nmi_backtrace_stall_snap(const struct cpumask *btp) {}
static inline void nmi_backtrace_stall_check(const struct cpumask *btp) {}

#endif
