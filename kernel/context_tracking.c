// SPDX-License-Identifier: GPL-2.0-only
/*
 * Context tracking: Probe on high level context boundaries such as kernel,
 * userspace, guest or idle.
 *
 * This is used by RCU to remove its dependency on the timer tick while a CPU
 * runs in idle, userspace or guest mode.
 *
 * User/guest tracking started by Frederic Weisbecker:
 *
 * Copyright (C) 2012 Red Hat, Inc., Frederic Weisbecker
 *
 * Many thanks to Gilad Ben-Yossef, Paul McKenney, Ingo Molnar, Andrew Morton,
 * Steven Rostedt, Peter Zijlstra for suggestions and improvements.
 *
 * RCU extended quiescent state bits imported from kernel/rcu/tree.c
 * where the relevant authorship may be found.
 */

#include <linux/context_tracking.h>
#include <linux/rcupdate.h>
#include <linux/sched.h>
#include <linux/hardirq.h>
#include <linux/export.h>
#include <linux/kprobes.h>


DEFINE_PER_CPU(struct context_tracking, context_tracking) = {
#ifdef CONFIG_CONTEXT_TRACKING_IDLE
	.nesting = 1,
	.nmi_nesting = CT_NESTING_IRQ_NONIDLE,
#endif
	.state = ATOMIC_INIT(CT_RCU_WATCHING),
};
EXPORT_SYMBOL_GPL(context_tracking);

#ifdef CONFIG_CONTEXT_TRACKING_IDLE
#define TPS(x)  tracepoint_string(x)

/* Record the current task on exiting RCU-tasks (dyntick-idle entry). */
static __always_inline void rcu_task_exit(void)
{
}

/* Record no current task on entering RCU-tasks (dyntick-idle exit). */
static __always_inline void rcu_task_enter(void)
{
}

/*
 * Record entry into an extended quiescent state.  This is only to be
 * called when not already in an extended quiescent state, that is,
 * RCU is watching prior to the call to this function and is no longer
 * watching upon return.
 */
static noinstr void ct_kernel_exit_state(int offset)
{
	/*
	 * CPUs seeing atomic_add_return() must see prior RCU read-side
	 * critical sections, and we also must force ordering with the
	 * next idle sojourn.
	 */
	(void)ct_state_inc(offset);
	// RCU is no longer watching.
}

/*
 * Record exit from an extended quiescent state.  This is only to be
 * called from an extended quiescent state, that is, RCU is not watching
 * prior to the call to this function and is watching upon return.
 */
static noinstr void ct_kernel_enter_state(int offset)
{
	/*
	 * CPUs seeing atomic_add_return() must see prior idle sojourns,
	 * and we also must force ordering with the next RCU read-side
	 * critical section.
	 */
	(void)ct_state_inc(offset);
}

/*
 * Enter an RCU extended quiescent state, which can be either the
 * idle loop or adaptive-tickless usermode execution.
 *
 * We crowbar the ->nmi_nesting field to zero to allow for
 * the possibility of usermode upcalls having messed up our count
 * of interrupt nesting level during the prior busy period.
 */
static void noinstr ct_kernel_exit(bool user, int offset)
{
	struct context_tracking *ct = this_cpu_ptr(&context_tracking);

	WARN_ON_ONCE(ct_nmi_nesting() != CT_NESTING_IRQ_NONIDLE);
	WRITE_ONCE(ct->nmi_nesting, 0);
	if (ct_nesting() != 1) {
		// RCU will still be watching, so just do accounting and leave.
		ct->nesting--;
		return;
	}

	instrumentation_begin();
	lockdep_assert_irqs_disabled();
	rcu_preempt_deferred_qs(current);

	// instrumentation for the noinstr ct_kernel_exit_state()
	instrument_atomic_write(&ct->state, sizeof(ct->state));

	instrumentation_end();
	WRITE_ONCE(ct->nesting, 0); /* Avoid irq-access tearing. */
	// RCU is watching here ...
	ct_kernel_exit_state(offset);
	// ... but is no longer watching here.
	rcu_task_exit();
}

/*
 * Exit an RCU extended quiescent state, which can be either the
 * idle loop or adaptive-tickless usermode execution.
 *
 * We crowbar the ->nmi_nesting field to CT_NESTING_IRQ_NONIDLE to
 * allow for the possibility of usermode upcalls messing up our count of
 * interrupt nesting level during the busy period that is just now starting.
 */
static void noinstr ct_kernel_enter(bool user, int offset)
{
	struct context_tracking *ct = this_cpu_ptr(&context_tracking);
	long oldval;

	oldval = ct_nesting();
	if (oldval) {
		// RCU was already watching, so just do accounting and leave.
		ct->nesting++;
		return;
	}
	rcu_task_enter();
	// RCU is not watching here ...
	ct_kernel_enter_state(offset);
	// ... but is watching here.
	instrumentation_begin();

	// instrumentation for the noinstr ct_kernel_enter_state()
	instrument_atomic_write(&ct->state, sizeof(ct->state));

	WRITE_ONCE(ct->nesting, 1);
	WARN_ON_ONCE(ct_nmi_nesting());
	WRITE_ONCE(ct->nmi_nesting, CT_NESTING_IRQ_NONIDLE);
	instrumentation_end();
}

/**
 * ct_nmi_exit - inform RCU of exit from NMI context
 *
 * If we are returning from the outermost NMI handler that interrupted an
 * RCU-idle period, update ct->state and ct->nmi_nesting
 * to let the RCU grace-period handling know that the CPU is back to
 * being RCU-idle.
 *
 * If you add or remove a call to ct_nmi_exit(), be sure to test
 * with CONFIG_RCU_EQS_DEBUG=y.
 */
void noinstr ct_nmi_exit(void)
{
	struct context_tracking *ct = this_cpu_ptr(&context_tracking);

	instrumentation_begin();
	/*
	 * Check for ->nmi_nesting underflow and bad CT state.
	 * (We are exiting an NMI handler, so RCU better be paying attention
	 * to us!)
	 */
	WARN_ON_ONCE(ct_nmi_nesting() <= 0);
	WARN_ON_ONCE(!rcu_is_watching_curr_cpu());

	/*
	 * If the nesting level is not 1, the CPU wasn't RCU-idle, so
	 * leave it in non-RCU-idle state.
	 */
	if (ct_nmi_nesting() != 1) {
		WRITE_ONCE(ct->nmi_nesting, /* No store tearing. */
			   ct_nmi_nesting() - 2);
		instrumentation_end();
		return;
	}

	/* This NMI interrupted an RCU-idle CPU, restore RCU-idleness. */
	WRITE_ONCE(ct->nmi_nesting, 0); /* Avoid store tearing. */

	// instrumentation for the noinstr ct_kernel_exit_state()
	instrument_atomic_write(&ct->state, sizeof(ct->state));
	instrumentation_end();

	// RCU is watching here ...
	ct_kernel_exit_state(CT_RCU_WATCHING);
	// ... but is no longer watching here.

	if (!in_nmi())
		rcu_task_exit();
}

/**
 * ct_nmi_enter - inform RCU of entry to NMI context
 *
 * If the CPU was idle from RCU's viewpoint, update ct->state and
 * ct->nmi_nesting to let the RCU grace-period handling know
 * that the CPU is active.  This implementation permits nested NMIs, as
 * long as the nesting level does not overflow an int.  (You will probably
 * run out of stack space first.)
 *
 * If you add or remove a call to ct_nmi_enter(), be sure to test
 * with CONFIG_RCU_EQS_DEBUG=y.
 */
void noinstr ct_nmi_enter(void)
{
	long incby = 2;
	struct context_tracking *ct = this_cpu_ptr(&context_tracking);

	/* Complain about underflow. */
	WARN_ON_ONCE(ct_nmi_nesting() < 0);

	/*
	 * If idle from RCU viewpoint, atomically increment CT state
	 * to mark non-idle and increment ->nmi_nesting by one.
	 * Otherwise, increment ->nmi_nesting by two.  This means
	 * if ->nmi_nesting is equal to one, we are guaranteed
	 * to be in the outermost NMI handler that interrupted an RCU-idle
	 * period (observation due to Andy Lutomirski).
	 */
	if (!rcu_is_watching_curr_cpu()) {

		if (!in_nmi())
			rcu_task_enter();

		// RCU is not watching here ...
		ct_kernel_enter_state(CT_RCU_WATCHING);
		// ... but is watching here.

		instrumentation_begin();
		// instrumentation for the noinstr rcu_is_watching_curr_cpu()
		instrument_atomic_read(&ct->state, sizeof(ct->state));
		// instrumentation for the noinstr ct_kernel_enter_state()
		instrument_atomic_write(&ct->state, sizeof(ct->state));

		incby = 1;
	} else if (!in_nmi()) {
		instrumentation_begin();
		rcu_irq_enter_check_tick();
	} else  {
		instrumentation_begin();
	}

	instrumentation_end();
	WRITE_ONCE(ct->nmi_nesting, /* Prevent store tearing. */
		   ct_nmi_nesting() + incby);
	barrier();
}

/**
 * ct_idle_enter - inform RCU that current CPU is entering idle
 *
 * Enter idle mode, in other words, -leave- the mode in which RCU
 * read-side critical sections can occur.  (Though RCU read-side
 * critical sections can occur in irq handlers in idle, a possibility
 * handled by irq_enter() and irq_exit().)
 *
 * If you add or remove a call to ct_idle_enter(), be sure to test with
 * CONFIG_RCU_EQS_DEBUG=y.
 */
void noinstr ct_idle_enter(void)
{
	ct_kernel_exit(false, CT_RCU_WATCHING + CT_STATE_IDLE);
}
EXPORT_SYMBOL_GPL(ct_idle_enter);

/**
 * ct_idle_exit - inform RCU that current CPU is leaving idle
 *
 * Exit idle mode, in other words, -enter- the mode in which RCU
 * read-side critical sections can occur.
 *
 * If you add or remove a call to ct_idle_exit(), be sure to test with
 * CONFIG_RCU_EQS_DEBUG=y.
 */
void noinstr ct_idle_exit(void)
{
	unsigned long flags;

	raw_local_irq_save(flags);
	ct_kernel_enter(false, CT_RCU_WATCHING - CT_STATE_IDLE);
	raw_local_irq_restore(flags);
}
EXPORT_SYMBOL_GPL(ct_idle_exit);

/**
 * ct_irq_enter - inform RCU that current CPU is entering irq away from idle
 *
 * Enter an interrupt handler, which might possibly result in exiting
 * idle mode, in other words, entering the mode in which read-side critical
 * sections can occur.  The caller must have disabled interrupts.
 *
 * Note that the Linux kernel is fully capable of entering an interrupt
 * handler that it never exits, for example when doing upcalls to user mode!
 * This code assumes that the idle loop never does upcalls to user mode.
 * If your architecture's idle loop does do upcalls to user mode (or does
 * anything else that results in unbalanced calls to the irq_enter() and
 * irq_exit() functions), RCU will give you what you deserve, good and hard.
 * But very infrequently and irreproducibly.
 *
 * Use things like work queues to work around this limitation.
 *
 * You have been warned.
 *
 * If you add or remove a call to ct_irq_enter(), be sure to test with
 * CONFIG_RCU_EQS_DEBUG=y.
 */
noinstr void ct_irq_enter(void)
{
	lockdep_assert_irqs_disabled();
	ct_nmi_enter();
}

/**
 * ct_irq_exit - inform RCU that current CPU is exiting irq towards idle
 *
 * Exit from an interrupt handler, which might possibly result in entering
 * idle mode, in other words, leaving the mode in which read-side critical
 * sections can occur.  The caller must have disabled interrupts.
 *
 * This code assumes that the idle loop never does anything that might
 * result in unbalanced calls to irq_enter() and irq_exit().  If your
 * architecture's idle loop violates this assumption, RCU will give you what
 * you deserve, good and hard.  But very infrequently and irreproducibly.
 *
 * Use things like work queues to work around this limitation.
 *
 * You have been warned.
 *
 * If you add or remove a call to ct_irq_exit(), be sure to test with
 * CONFIG_RCU_EQS_DEBUG=y.
 */
noinstr void ct_irq_exit(void)
{
	lockdep_assert_irqs_disabled();
	ct_nmi_exit();
}

/*
 * Wrapper for ct_irq_enter() where interrupts are enabled.
 *
 * If you add or remove a call to ct_irq_enter_irqson(), be sure to test
 * with CONFIG_RCU_EQS_DEBUG=y.
 */
void ct_irq_enter_irqson(void)
{
	unsigned long flags;

	local_irq_save(flags);
	ct_irq_enter();
	local_irq_restore(flags);
}

/*
 * Wrapper for ct_irq_exit() where interrupts are enabled.
 *
 * If you add or remove a call to ct_irq_exit_irqson(), be sure to test
 * with CONFIG_RCU_EQS_DEBUG=y.
 */
void ct_irq_exit_irqson(void)
{
	unsigned long flags;

	local_irq_save(flags);
	ct_irq_exit();
	local_irq_restore(flags);
}
#else
static __always_inline void ct_kernel_exit(bool user, int offset) { }
static __always_inline void ct_kernel_enter(bool user, int offset) { }
#endif /* #ifdef CONFIG_CONTEXT_TRACKING_IDLE */
