// SPDX-License-Identifier: GPL-2.0
/*
 * Detect hard and soft lockups on a system
 *
 * started by Don Zickus, Copyright (C) 2010 Red Hat, Inc.
 *
 * Note: Most of this code is borrowed heavily from the original softlockup
 * detector, so thanks to Ingo for the initial implementation.
 * Some chunks also taken from the old x86-specific nmi watchdog code, thanks
 * to those contributors as well.
 */

#define pr_fmt(fmt) "watchdog: " fmt

#include <linux/cpu.h>
#include <linux/init.h>
#include <linux/irq.h>
#include <linux/irqdesc.h>
#include <linux/kernel_stat.h>
#include <linux/kvm_para.h>
#include <linux/math64.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/nmi.h>
#include <linux/stop_machine.h>
#include <linux/sysctl.h>
#include <linux/tick.h>
#include <linux/sys_info.h>

#include <linux/sched/clock.h>
#include <linux/sched/debug.h>
#include <linux/sched/isolation.h>

#include <asm/irq_regs.h>

static DEFINE_MUTEX(watchdog_mutex);

#if defined(CONFIG_HARDLOCKUP_DETECTOR) || defined(CONFIG_HARDLOCKUP_DETECTOR_SPARC64)
# define WATCHDOG_HARDLOCKUP_DEFAULT	1
#else
# define WATCHDOG_HARDLOCKUP_DEFAULT	0
#endif

#define NUM_SAMPLE_PERIODS	5

unsigned long __read_mostly watchdog_enabled;
int __read_mostly watchdog_user_enabled = 1;
static int __read_mostly watchdog_hardlockup_user_enabled = WATCHDOG_HARDLOCKUP_DEFAULT;
static int __read_mostly watchdog_softlockup_user_enabled = 1;
int __read_mostly watchdog_thresh = 10;
static int __read_mostly watchdog_thresh_next;
static int __read_mostly watchdog_hardlockup_available;

struct cpumask watchdog_cpumask __read_mostly;
unsigned long *watchdog_cpumask_bits = cpumask_bits(&watchdog_cpumask);


#if defined(CONFIG_HARDLOCKUP_DETECTOR_COUNTS_HRTIMER)

static DEFINE_PER_CPU(atomic_t, hrtimer_interrupts);
static DEFINE_PER_CPU(int, hrtimer_interrupts_saved);
static DEFINE_PER_CPU(int, hrtimer_interrupts_missed);
static DEFINE_PER_CPU(bool, watchdog_hardlockup_warned);
static DEFINE_PER_CPU(bool, watchdog_hardlockup_touched);
static unsigned long hard_lockup_nmi_warn;

notrace void arch_touch_nmi_watchdog(void)
{
	/*
	 * Using __raw here because some code paths have
	 * preemption enabled.  If preemption is enabled
	 * then interrupts should be enabled too, in which
	 * case we shouldn't have to worry about the watchdog
	 * going off.
	 */
	raw_cpu_write(watchdog_hardlockup_touched, true);
}
EXPORT_SYMBOL(arch_touch_nmi_watchdog);

void watchdog_hardlockup_touch_cpu(unsigned int cpu)
{
	per_cpu(watchdog_hardlockup_touched, cpu) = true;
}

static void watchdog_hardlockup_update_reset(unsigned int cpu)
{
	int hrint = atomic_read(&per_cpu(hrtimer_interrupts, cpu));

	/*
	 * NOTE: we don't need any fancy atomic_t or READ_ONCE/WRITE_ONCE
	 * for hrtimer_interrupts_saved. hrtimer_interrupts_saved is
	 * written/read by a single CPU.
	 */
	per_cpu(hrtimer_interrupts_saved, cpu) = hrint;
	per_cpu(hrtimer_interrupts_missed, cpu) = 0;
}

static bool is_hardlockup(unsigned int cpu)
{
	int hrint = atomic_read(&per_cpu(hrtimer_interrupts, cpu));

	if (per_cpu(hrtimer_interrupts_saved, cpu) != hrint) {
		watchdog_hardlockup_update_reset(cpu);
		return false;
	}

	per_cpu(hrtimer_interrupts_missed, cpu)++;
	if (per_cpu(hrtimer_interrupts_missed, cpu) % watchdog_hardlockup_miss_thresh)
		return false;

	return true;
}

static void watchdog_hardlockup_kick(void)
{
	int new_interrupts;

	new_interrupts = atomic_inc_return(this_cpu_ptr(&hrtimer_interrupts));
	watchdog_buddy_check_hardlockup(new_interrupts);
}

void watchdog_hardlockup_check(unsigned int cpu, struct pt_regs *regs)
{
	int hardlockup_all_cpu_backtrace;
	unsigned int this_cpu;
	unsigned long flags;

	if (per_cpu(watchdog_hardlockup_touched, cpu)) {
		watchdog_hardlockup_update_reset(cpu);
		per_cpu(watchdog_hardlockup_touched, cpu) = false;
		return;
	}

	hardlockup_all_cpu_backtrace = (hardlockup_si_mask & SYS_INFO_ALL_BT) ?
					1 : sysctl_hardlockup_all_cpu_backtrace;
	/*
	 * Check for a hardlockup by making sure the CPU's timer
	 * interrupt is incrementing. The timer interrupt should have
	 * fired multiple times before we overflow'd. If it hasn't
	 * then this is a good indication the cpu is stuck
	 */
	if (!is_hardlockup(cpu)) {
		per_cpu(watchdog_hardlockup_warned, cpu) = false;
		return;
	}

#ifdef CONFIG_SYSFS
	++hardlockup_count;
#endif
	/*
	 * A poorly behaving BPF scheduler can trigger hard lockup by
	 * e.g. putting numerous affinitized tasks in a single queue and
	 * directing all CPUs at it. The following call can return true
	 * only once when sched_ext is enabled and will immediately
	 * abort the BPF scheduler and print out a warning message.
	 */
	if (scx_hardlockup(cpu))
		return;

	/* Only print hardlockups once. */
	if (per_cpu(watchdog_hardlockup_warned, cpu))
		return;

	/*
	 * Prevent multiple hard-lockup reports if one cpu is already
	 * engaged in dumping all cpu back traces.
	 */
	if (hardlockup_all_cpu_backtrace) {
		if (test_and_set_bit_lock(0, &hard_lockup_nmi_warn))
			return;
	}

	/*
	 * NOTE: we call printk_cpu_sync_get_irqsave() after printing
	 * the lockup message. While it would be nice to serialize
	 * that printout, we really want to make sure that if some
	 * other CPU somehow locked up while holding the lock associated
	 * with printk_cpu_sync_get_irqsave() that we can still at least
	 * get the message about the lockup out.
	 */
	this_cpu = smp_processor_id();
	pr_emerg("CPU%u: Watchdog detected hard LOCKUP on cpu %u\n", this_cpu, cpu);
	printk_cpu_sync_get_irqsave(flags);

	print_modules();
	print_irqtrace_events(current);
	if (cpu == this_cpu) {
		if (regs)
			show_regs(regs);
		else
			dump_stack();
		printk_cpu_sync_put_irqrestore(flags);
	} else {
		printk_cpu_sync_put_irqrestore(flags);
		trigger_single_cpu_backtrace(cpu);
	}

	if (hardlockup_all_cpu_backtrace) {
		trigger_allbutcpu_cpu_backtrace(cpu);
		if (!hardlockup_panic)
			clear_bit_unlock(0, &hard_lockup_nmi_warn);
	}

	sys_info(hardlockup_si_mask & ~SYS_INFO_ALL_BT);
	if (hardlockup_panic)
		nmi_panic(regs, "Hard LOCKUP");

	per_cpu(watchdog_hardlockup_warned, cpu) = true;
}

#else /* CONFIG_HARDLOCKUP_DETECTOR_COUNTS_HRTIMER */

static inline void watchdog_hardlockup_kick(void) { }

#endif /* !CONFIG_HARDLOCKUP_DETECTOR_COUNTS_HRTIMER */

/*
 * These functions can be overridden based on the configured hardlockdup detector.
 *
 * watchdog_hardlockup_enable/disable can be implemented to start and stop when
 * softlockup watchdog start and stop. The detector must select the
 * SOFTLOCKUP_DETECTOR Kconfig.
 */
void __weak watchdog_hardlockup_enable(unsigned int cpu) { }

void __weak watchdog_hardlockup_disable(unsigned int cpu) { }

/*
 * Watchdog-detector specific API.
 *
 * Return 0 when hardlockup watchdog is available, negative value otherwise.
 * Note that the negative value means that a delayed probe might
 * succeed later.
 */
int __weak __init watchdog_hardlockup_probe(void)
{
	return -ENODEV;
}

/**
 * watchdog_hardlockup_stop - Stop the watchdog for reconfiguration
 *
 * The reconfiguration steps are:
 * watchdog_hardlockup_stop();
 * update_variables();
 * watchdog_hardlockup_start();
 */
void __weak watchdog_hardlockup_stop(void) { }

/**
 * watchdog_hardlockup_start - Start the watchdog after reconfiguration
 *
 * Counterpart to watchdog_hardlockup_stop().
 *
 * The following variables have been updated in update_variables() and
 * contain the currently valid configuration:
 * - watchdog_enabled
 * - watchdog_thresh
 * - watchdog_cpumask
 */
void __weak watchdog_hardlockup_start(void) { }

/**
 * lockup_detector_update_enable - Update the sysctl enable bit
 *
 * Caller needs to make sure that the hard watchdogs are off, so this
 * can't race with watchdog_hardlockup_disable().
 */
static void lockup_detector_update_enable(void)
{
	watchdog_enabled = 0;
	if (!watchdog_user_enabled)
		return;
	if (watchdog_hardlockup_available && watchdog_hardlockup_user_enabled)
		watchdog_enabled |= WATCHDOG_HARDLOCKUP_ENABLED;
	if (watchdog_softlockup_user_enabled)
		watchdog_enabled |= WATCHDOG_SOFTOCKUP_ENABLED;
}

static void __lockup_detector_reconfigure(bool thresh_changed)
{
	cpus_read_lock();
	watchdog_hardlockup_stop();
	if (thresh_changed)
		watchdog_thresh = READ_ONCE(watchdog_thresh_next);
	lockup_detector_update_enable();
	watchdog_hardlockup_start();
	cpus_read_unlock();
}
void lockup_detector_reconfigure(void)
{
	__lockup_detector_reconfigure(false);
}
static inline void lockup_detector_setup(void)
{
	__lockup_detector_reconfigure(false);
}

/**
 * lockup_detector_soft_poweroff - Interface to stop lockup detector(s)
 *
 * Special interface for parisc. It prevents lockup detector warnings from
 * the default pm_poweroff() function which busy loops forever.
 */
void lockup_detector_soft_poweroff(void)
{
	watchdog_enabled = 0;
}

#ifdef CONFIG_SYSCTL

/* Propagate any changes to the watchdog infrastructure */
static void proc_watchdog_update(bool thresh_changed)
{
	/* Remove impossible cpus to keep sysctl output clean. */
	cpumask_and(&watchdog_cpumask, &watchdog_cpumask, cpu_possible_mask);
	__lockup_detector_reconfigure(thresh_changed);
}

/*
 * common function for watchdog, nmi_watchdog and soft_watchdog parameter
 *
 * caller             | table->data points to            | 'which'
 * -------------------|----------------------------------|-------------------------------
 * proc_watchdog      | watchdog_user_enabled            | WATCHDOG_HARDLOCKUP_ENABLED |
 *                    |                                  | WATCHDOG_SOFTOCKUP_ENABLED
 * -------------------|----------------------------------|-------------------------------
 * proc_nmi_watchdog  | watchdog_hardlockup_user_enabled | WATCHDOG_HARDLOCKUP_ENABLED
 * -------------------|----------------------------------|-------------------------------
 * proc_soft_watchdog | watchdog_softlockup_user_enabled | WATCHDOG_SOFTOCKUP_ENABLED
 */
static int proc_watchdog_common(int which, const struct ctl_table *table, int write,
				void *buffer, size_t *lenp, loff_t *ppos)
{
	int err, old, *param = table->data;

	mutex_lock(&watchdog_mutex);

	old = *param;
	if (!write) {
		/*
		 * On read synchronize the userspace interface. This is a
		 * racy snapshot.
		 */
		*param = (watchdog_enabled & which) != 0;
		err = proc_dointvec_minmax(table, write, buffer, lenp, ppos);
		*param = old;
	} else {
		err = proc_dointvec_minmax(table, write, buffer, lenp, ppos);
		if (!err && old != READ_ONCE(*param))
			proc_watchdog_update(false);
	}
	mutex_unlock(&watchdog_mutex);
	return err;
}

/*
 * /proc/sys/kernel/watchdog
 */
static int proc_watchdog(const struct ctl_table *table, int write,
			 void *buffer, size_t *lenp, loff_t *ppos)
{
	return proc_watchdog_common(WATCHDOG_HARDLOCKUP_ENABLED |
				    WATCHDOG_SOFTOCKUP_ENABLED,
				    table, write, buffer, lenp, ppos);
}

/*
 * /proc/sys/kernel/nmi_watchdog
 */
static int proc_nmi_watchdog(const struct ctl_table *table, int write,
			     void *buffer, size_t *lenp, loff_t *ppos)
{
	if (!watchdog_hardlockup_available && write)
		return -ENOTSUPP;
	return proc_watchdog_common(WATCHDOG_HARDLOCKUP_ENABLED,
				    table, write, buffer, lenp, ppos);
}


/*
 * /proc/sys/kernel/watchdog_thresh
 */
static int proc_watchdog_thresh(const struct ctl_table *table, int write,
				void *buffer, size_t *lenp, loff_t *ppos)
{
	int err, old;

	mutex_lock(&watchdog_mutex);

	watchdog_thresh_next = READ_ONCE(watchdog_thresh);

	old = watchdog_thresh_next;
	err = proc_dointvec_minmax(table, write, buffer, lenp, ppos);

	if (!err && write && old != READ_ONCE(watchdog_thresh_next))
		proc_watchdog_update(true);

	mutex_unlock(&watchdog_mutex);
	return err;
}

/*
 * The cpumask is the mask of possible cpus that the watchdog can run
 * on, not the mask of cpus it is actually running on.  This allows the
 * user to specify a mask that will include cpus that have not yet
 * been brought online, if desired.
 */
static int proc_watchdog_cpumask(const struct ctl_table *table, int write,
				 void *buffer, size_t *lenp, loff_t *ppos)
{
	int err;

	mutex_lock(&watchdog_mutex);

	err = proc_do_large_bitmap(table, write, buffer, lenp, ppos);
	if (!err && write)
		proc_watchdog_update(false);

	mutex_unlock(&watchdog_mutex);
	return err;
}

static const int sixty = 60;

static const struct ctl_table watchdog_sysctls[] = {
	{
		.procname       = "watchdog",
		.data		= &watchdog_user_enabled,
		.maxlen		= sizeof(int),
		.mode		= 0644,
		.proc_handler   = proc_watchdog,
		.extra1		= SYSCTL_ZERO,
		.extra2		= SYSCTL_ONE,
	},
	{
		.procname	= "watchdog_thresh",
		.data		= &watchdog_thresh_next,
		.maxlen		= sizeof(int),
		.mode		= 0644,
		.proc_handler	= proc_watchdog_thresh,
		.extra1		= SYSCTL_ZERO,
		.extra2		= (void *)&sixty,
	},
	{
		.procname	= "watchdog_cpumask",
		.data		= &watchdog_cpumask_bits,
		.maxlen		= NR_CPUS,
		.mode		= 0644,
		.proc_handler	= proc_watchdog_cpumask,
	},
	{
		.procname       = "nmi_watchdog",
		.data		= &watchdog_hardlockup_user_enabled,
		.maxlen		= sizeof(int),
		.mode		= 0644,
		.proc_handler   = proc_nmi_watchdog,
		.extra1		= SYSCTL_ZERO,
		.extra2		= SYSCTL_ONE,
	},
};

static void __init watchdog_sysctl_init(void)
{
	register_sysctl_init("kernel", watchdog_sysctls);
}

#else
#define watchdog_sysctl_init() do { } while (0)
#endif /* CONFIG_SYSCTL */

static void __init lockup_detector_delay_init(struct work_struct *work);
static bool allow_lockup_detector_init_retry __initdata;

static struct work_struct detector_work __initdata =
		__WORK_INITIALIZER(detector_work, lockup_detector_delay_init);

static void __init lockup_detector_delay_init(struct work_struct *work)
{
	int ret;

	ret = watchdog_hardlockup_probe();
	if (ret) {
		if (ret == -ENODEV)
			pr_info("NMI not fully supported\n");
		else
			pr_info("Delayed init of the lockup detector failed: %d\n", ret);
		pr_info("Hard watchdog permanently disabled\n");
		return;
	}

	allow_lockup_detector_init_retry = false;

	watchdog_hardlockup_available = true;
	lockup_detector_setup();
}

/*
 * lockup_detector_retry_init - retry init lockup detector if possible.
 *
 * Retry hardlockup detector init. It is useful when it requires some
 * functionality that has to be initialized later on a particular
 * platform.
 */
void __init lockup_detector_retry_init(void)
{
	/* Must be called before late init calls */
	if (!allow_lockup_detector_init_retry)
		return;

	schedule_work(&detector_work);
}

/*
 * Ensure that optional delayed hardlockup init is proceed before
 * the init code and memory is freed.
 */
static int __init lockup_detector_check(void)
{
	/* Prevent any later retry. */
	allow_lockup_detector_init_retry = false;

	/* Make sure no work is pending. */
	flush_work(&detector_work);

	watchdog_sysctl_init();

	return 0;

}
late_initcall_sync(lockup_detector_check);

void __init lockup_detector_init(void)
{
	if (tick_nohz_full_enabled())
		pr_info("Disabling watchdog on nohz_full cores by default\n");

	cpumask_copy(&watchdog_cpumask,
		     housekeeping_cpumask(HK_TYPE_TIMER));

	if (!watchdog_hardlockup_probe())
		watchdog_hardlockup_available = true;
	else
		allow_lockup_detector_init_retry = true;

	lockup_detector_setup();
}
