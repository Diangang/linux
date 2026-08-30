/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_CONTEXT_TRACKING_H
#define _LINUX_CONTEXT_TRACKING_H

#include <linux/sched.h>
#include <linux/vtime.h>
#include <linux/context_tracking_state.h>
#include <linux/instrumentation.h>

#include <asm/ptrace.h>


static inline void user_enter(void) { }
static inline void user_exit(void) { }
static inline void user_enter_irqoff(void) { }
static inline void user_exit_irqoff(void) { }
static inline int exception_enter(void) { return 0; }
static inline void exception_exit(enum ctx_state prev_ctx) { }
static inline int ct_state(void) { return -1; }
static inline int __ct_state(void) { return -1; }
static __always_inline bool context_tracking_guest_enter(void) { return false; }
static __always_inline bool context_tracking_guest_exit(void) { return false; }
#define CT_WARN_ON(cond) do { } while (0)

static inline void context_tracking_init(void) { }

#ifdef CONFIG_CONTEXT_TRACKING_IDLE
extern void ct_idle_enter(void);
extern void ct_idle_exit(void);

/*
 * Is RCU watching the current CPU (IOW, it is not in an extended quiescent state)?
 *
 * Note that this returns the actual boolean data (watching / not watching),
 * whereas ct_rcu_watching() returns the RCU_WATCHING subvariable of
 * context_tracking.state.
 *
 * No ordering, as we are sampling CPU-local information.
 */
static __always_inline bool rcu_is_watching_curr_cpu(void)
{
	return raw_atomic_read(this_cpu_ptr(&context_tracking.state)) & CT_RCU_WATCHING;
}

/*
 * Increment the current CPU's context_tracking structure's ->state field
 * with ordering.  Return the new value.
 */
static __always_inline unsigned long ct_state_inc(int incby)
{
	return raw_atomic_add_return(incby, this_cpu_ptr(&context_tracking.state));
}

static __always_inline bool warn_rcu_enter(void)
{
	bool ret = false;

	/*
	 * Horrible hack to shut up recursive RCU isn't watching fail since
	 * lots of the actual reporting also relies on RCU.
	 */
	preempt_disable_notrace();
	if (!rcu_is_watching_curr_cpu()) {
		ret = true;
		ct_state_inc(CT_RCU_WATCHING);
	}

	return ret;
}

static __always_inline void warn_rcu_exit(bool rcu)
{
	if (rcu)
		ct_state_inc(CT_RCU_WATCHING);
	preempt_enable_notrace();
}

#else
static inline void ct_idle_enter(void) { }
static inline void ct_idle_exit(void) { }

static __always_inline bool warn_rcu_enter(void) { return false; }
static __always_inline void warn_rcu_exit(bool rcu) { }
#endif /* !CONFIG_CONTEXT_TRACKING_IDLE */

#endif
