// SPDX-License-Identifier: GPL-2.0+
/*
 * Read-Copy Update mechanism for mutual exclusion
 *
 * Copyright IBM Corporation, 2001
 *
 * Authors: Dipankar Sarma <dipankar@in.ibm.com>
 *	    Manfred Spraul <manfred@colorfullife.com>
 *
 * Based on the original work by Paul McKenney <paulmck@linux.ibm.com>
 * and inputs from Rusty Russell, Andrea Arcangeli and Andi Kleen.
 * Papers:
 * http://www.rdrop.com/users/paulmck/paper/rclockpdcsproof.pdf
 * http://lse.sourceforge.net/locking/rclock_OLS.2001.05.01c.sc.pdf (OLS2001)
 *
 * For detailed explanation of Read-Copy Update mechanism see -
 *		http://lse.sourceforge.net/locking/rcupdate.html
 *
 */
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/spinlock.h>
#include <linux/smp.h>
#include <linux/interrupt.h>
#include <linux/sched/signal.h>
#include <linux/sched/debug.h>
#include <linux/torture.h>
#include <linux/atomic.h>
#include <linux/bitops.h>
#include <linux/percpu.h>
#include <linux/notifier.h>
#include <linux/cpu.h>
#include <linux/mutex.h>
#include <linux/export.h>
#include <linux/hardirq.h>
#include <linux/delay.h>
#include <linux/moduleparam.h>
#include <linux/kthread.h>
#include <linux/tick.h>
#include <linux/rcupdate_wait.h>
#include <linux/sched/isolation.h>
#include <linux/kprobes.h>
#include <linux/slab.h>
#include <linux/irq_work.h>
#include <linux/rcupdate_trace.h>

#define CREATE_TRACE_POINTS

#include "rcu.h"

#ifdef MODULE_PARAM_PREFIX
#undef MODULE_PARAM_PREFIX
#endif
#define MODULE_PARAM_PREFIX "rcupdate."

#ifndef CONFIG_TINY_RCU
module_param(rcu_expedited, int, 0444);
module_param(rcu_normal, int, 0444);
static int rcu_normal_after_boot = IS_ENABLED(CONFIG_PREEMPT_RT);
#if !defined(CONFIG_PREEMPT_RT) || defined(CONFIG_NO_HZ_FULL)
module_param(rcu_normal_after_boot, int, 0444);
#endif
#endif /* #ifndef CONFIG_TINY_RCU */


#ifndef CONFIG_TINY_RCU

/*
 * Should expedited grace-period primitives always fall back to their
 * non-expedited counterparts?  Intended for use within RCU.  Note
 * that if the user specifies both rcu_expedited and rcu_normal, then
 * rcu_normal wins.  (Except during the time period during boot from
 * when the first task is spawned until the rcu_set_runtime_mode()
 * core_initcall() is invoked, at which point everything is expedited.)
 */
bool rcu_gp_is_normal(void)
{
	return READ_ONCE(rcu_normal) &&
	       rcu_scheduler_active != RCU_SCHEDULER_INIT;
}
EXPORT_SYMBOL_GPL(rcu_gp_is_normal);

static atomic_t rcu_async_hurry_nesting = ATOMIC_INIT(1);
/*
 * Should call_rcu() callbacks be processed with urgency or are
 * they OK being executed with arbitrary delays?
 */
bool rcu_async_should_hurry(void)
{
	return !IS_ENABLED(CONFIG_RCU_LAZY) ||
	       atomic_read(&rcu_async_hurry_nesting);
}
EXPORT_SYMBOL_GPL(rcu_async_should_hurry);

/**
 * rcu_async_hurry - Make future async RCU callbacks not lazy.
 *
 * After a call to this function, future calls to call_rcu()
 * will be processed in a timely fashion.
 */
void rcu_async_hurry(void)
{
	if (IS_ENABLED(CONFIG_RCU_LAZY))
		atomic_inc(&rcu_async_hurry_nesting);
}
EXPORT_SYMBOL_GPL(rcu_async_hurry);

/**
 * rcu_async_relax - Make future async RCU callbacks lazy.
 *
 * After a call to this function, future calls to call_rcu()
 * will be processed in a lazy fashion.
 */
void rcu_async_relax(void)
{
	if (IS_ENABLED(CONFIG_RCU_LAZY))
		atomic_dec(&rcu_async_hurry_nesting);
}
EXPORT_SYMBOL_GPL(rcu_async_relax);

static atomic_t rcu_expedited_nesting = ATOMIC_INIT(1);
/*
 * Should normal grace-period primitives be expedited?  Intended for
 * use within RCU.  Note that this function takes the rcu_expedited
 * sysfs/boot variable and rcu_scheduler_active into account as well
 * as the rcu_expedite_gp() nesting.  So looping on rcu_unexpedite_gp()
 * until rcu_gp_is_expedited() returns false is a -really- bad idea.
 */
bool rcu_gp_is_expedited(void)
{
	return rcu_expedited || atomic_read(&rcu_expedited_nesting);
}
EXPORT_SYMBOL_GPL(rcu_gp_is_expedited);

/**
 * rcu_expedite_gp - Expedite future RCU grace periods
 *
 * After a call to this function, future calls to synchronize_rcu() and
 * friends act as the corresponding synchronize_rcu_expedited() function
 * had instead been called.
 */
void rcu_expedite_gp(void)
{
	atomic_inc(&rcu_expedited_nesting);
}
EXPORT_SYMBOL_GPL(rcu_expedite_gp);

/**
 * rcu_unexpedite_gp - Cancel prior rcu_expedite_gp() invocation
 *
 * Undo a prior call to rcu_expedite_gp().  If all prior calls to
 * rcu_expedite_gp() are undone by a subsequent call to rcu_unexpedite_gp(),
 * and if the rcu_expedited sysfs/boot parameter is not set, then all
 * subsequent calls to synchronize_rcu() and friends will return to
 * their normal non-expedited behavior.
 */
void rcu_unexpedite_gp(void)
{
	atomic_dec(&rcu_expedited_nesting);
}
EXPORT_SYMBOL_GPL(rcu_unexpedite_gp);

static bool rcu_boot_ended __read_mostly;

/*
 * Inform RCU of the end of the in-kernel boot sequence.
 */
void rcu_end_inkernel_boot(void)
{
	rcu_unexpedite_gp();
	rcu_async_relax();
	if (rcu_normal_after_boot)
		WRITE_ONCE(rcu_normal, 1);
	rcu_boot_ended = true;
}

/*
 * Let rcutorture know when it is OK to turn it up to eleven.
 */
bool rcu_inkernel_boot_has_ended(void)
{
	return rcu_boot_ended;
}
EXPORT_SYMBOL_GPL(rcu_inkernel_boot_has_ended);

#endif /* #ifndef CONFIG_TINY_RCU */

/*
 * Test each non-SRCU synchronous grace-period wait API.  This is
 * useful just after a change in mode for these primitives, and
 * during early boot.
 */
void rcu_test_sync_prims(void)
{
	if (!IS_ENABLED(CONFIG_PROVE_RCU))
		return;
	pr_info("Running RCU synchronous self tests\n");
	synchronize_rcu();
	synchronize_rcu_expedited();
}

#if !defined(CONFIG_TINY_RCU)

/*
 * Switch to run-time mode once RCU has fully initialized.
 */
static int __init rcu_set_runtime_mode(void)
{
	rcu_test_sync_prims();
	rcu_scheduler_active = RCU_SCHEDULER_RUNNING;
	kfree_rcu_scheduler_running();
	rcu_test_sync_prims();
	return 0;
}
core_initcall(rcu_set_runtime_mode);

#endif /* #if !defined(CONFIG_TINY_RCU) */


/**
 * wakeme_after_rcu() - Callback function to awaken a task after grace period
 * @head: Pointer to rcu_head member within rcu_synchronize structure
 *
 * Awaken the corresponding task now that a grace period has elapsed.
 */
void wakeme_after_rcu(struct rcu_head *head)
{
	struct rcu_synchronize *rcu;

	rcu = container_of(head, struct rcu_synchronize, head);
	complete(&rcu->completion);
}
EXPORT_SYMBOL_GPL(wakeme_after_rcu);

void __wait_rcu_gp(bool checktiny, unsigned int state, int n, call_rcu_func_t *crcu_array,
		   struct rcu_synchronize *rs_array)
{
	int i;
	int j;

	/* Initialize and register callbacks for each crcu_array element. */
	for (i = 0; i < n; i++) {
		if (checktiny &&
		    (crcu_array[i] == call_rcu)) {
			might_sleep();
			continue;
		}
		for (j = 0; j < i; j++)
			if (crcu_array[j] == crcu_array[i])
				break;
		if (j == i) {
			init_rcu_head_on_stack(&rs_array[i].head);
			init_completion(&rs_array[i].completion);
			(crcu_array[i])(&rs_array[i].head, wakeme_after_rcu);
		}
	}

	/* Wait for all callbacks to be invoked. */
	for (i = 0; i < n; i++) {
		if (checktiny &&
		    (crcu_array[i] == call_rcu))
			continue;
		for (j = 0; j < i; j++)
			if (crcu_array[j] == crcu_array[i])
				break;
		if (j == i) {
			wait_for_completion_state(&rs_array[i].completion, state);
			destroy_rcu_head_on_stack(&rs_array[i].head);
		}
	}
}
EXPORT_SYMBOL_GPL(__wait_rcu_gp);

void finish_rcuwait(struct rcuwait *w)
{
	rcu_assign_pointer(w->task, NULL);
	__set_current_state(TASK_RUNNING);
}
EXPORT_SYMBOL_GPL(finish_rcuwait);

#ifdef CONFIG_DEBUG_OBJECTS_RCU_HEAD
void init_rcu_head(struct rcu_head *head)
{
	debug_object_init(head, &rcuhead_debug_descr);
}
EXPORT_SYMBOL_GPL(init_rcu_head);

void destroy_rcu_head(struct rcu_head *head)
{
	debug_object_free(head, &rcuhead_debug_descr);
}
EXPORT_SYMBOL_GPL(destroy_rcu_head);

static bool rcuhead_is_static_object(void *addr)
{
	return true;
}

/**
 * init_rcu_head_on_stack() - initialize on-stack rcu_head for debugobjects
 * @head: pointer to rcu_head structure to be initialized
 *
 * This function informs debugobjects of a new rcu_head structure that
 * has been allocated as an auto variable on the stack.  This function
 * is not required for rcu_head structures that are statically defined or
 * that are dynamically allocated on the heap.  This function has no
 * effect for !CONFIG_DEBUG_OBJECTS_RCU_HEAD kernel builds.
 */
void init_rcu_head_on_stack(struct rcu_head *head)
{
	debug_object_init_on_stack(head, &rcuhead_debug_descr);
}
EXPORT_SYMBOL_GPL(init_rcu_head_on_stack);

/**
 * destroy_rcu_head_on_stack() - destroy on-stack rcu_head for debugobjects
 * @head: pointer to rcu_head structure to be initialized
 *
 * This function informs debugobjects that an on-stack rcu_head structure
 * is about to go out of scope.  As with init_rcu_head_on_stack(), this
 * function is not required for rcu_head structures that are statically
 * defined or that are dynamically allocated on the heap.  Also as with
 * init_rcu_head_on_stack(), this function has no effect for
 * !CONFIG_DEBUG_OBJECTS_RCU_HEAD kernel builds.
 */
void destroy_rcu_head_on_stack(struct rcu_head *head)
{
	debug_object_free(head, &rcuhead_debug_descr);
}
EXPORT_SYMBOL_GPL(destroy_rcu_head_on_stack);

const struct debug_obj_descr rcuhead_debug_descr = {
	.name = "rcu_head",
	.is_static_object = rcuhead_is_static_object,
};
EXPORT_SYMBOL_GPL(rcuhead_debug_descr);
#endif /* #ifdef CONFIG_DEBUG_OBJECTS_RCU_HEAD */

#if defined(CONFIG_TREE_RCU) || defined(CONFIG_RCU_TRACE)
void do_trace_rcu_torture_read(const char *rcutorturename, struct rcu_head *rhp,
			       unsigned long secs,
			       unsigned long c_old, unsigned long c)
{
	trace_rcu_torture_read(rcutorturename, rhp, secs, c_old, c);
}
EXPORT_SYMBOL_GPL(do_trace_rcu_torture_read);
#else
#define do_trace_rcu_torture_read(rcutorturename, rhp, secs, c_old, c) \
	do { } while (0)
#endif

#if IS_ENABLED(CONFIG_RCU_TORTURE_TEST) || IS_MODULE(CONFIG_RCU_TORTURE_TEST) || IS_ENABLED(CONFIG_LOCK_TORTURE_TEST) || IS_MODULE(CONFIG_LOCK_TORTURE_TEST)
/* Get rcutorture access to sched_setaffinity(). */
long torture_sched_setaffinity(pid_t pid, const struct cpumask *in_mask, bool dowarn)
{
	int ret;

	ret = sched_setaffinity(pid, in_mask);
	WARN_ONCE(dowarn && ret, "%s: sched_setaffinity(%d) returned %d\n", __func__, pid, ret);
	return ret;
}
EXPORT_SYMBOL_GPL(torture_sched_setaffinity);
#endif

#if IS_ENABLED(CONFIG_TRIVIAL_PREEMPT_RCU)
// Trivial and stupid grace-period wait.  Defined here so that lockdep
// kernels can find tasklist_lock.
void synchronize_rcu_trivial_preempt(void)
{
	struct task_struct *g;
	struct task_struct *t;

	smp_mb(); // Order prior accesses before grace-period start.
	rcu_read_lock(); // Protect task list.
	for_each_process_thread(g, t) {
		if (t == current)
			continue;  // Don't deadlock on ourselves!
		// Order later rcu_read_lock() on other tasks after QS.
		while (smp_load_acquire(&t->rcu_trivial_preempt_nesting))
			continue;
	}
	rcu_read_unlock();
}
EXPORT_SYMBOL_GPL(synchronize_rcu_trivial_preempt);
#endif // #if IS_ENABLED(CONFIG_TRIVIAL_PREEMPT_RCU)

int rcu_cpu_stall_notifiers __read_mostly; // !0 = provide stall notifiers (rarely useful)
EXPORT_SYMBOL_GPL(rcu_cpu_stall_notifiers);

#ifdef CONFIG_RCU_STALL_COMMON
int rcu_cpu_stall_ftrace_dump __read_mostly;
module_param(rcu_cpu_stall_ftrace_dump, int, 0644);
#ifdef CONFIG_RCU_CPU_STALL_NOTIFIER
module_param(rcu_cpu_stall_notifiers, int, 0444);
#endif // #ifdef CONFIG_RCU_CPU_STALL_NOTIFIER
int rcu_cpu_stall_suppress __read_mostly; // !0 = suppress stall warnings.
EXPORT_SYMBOL_GPL(rcu_cpu_stall_suppress);
module_param(rcu_cpu_stall_suppress, int, 0644);
int rcu_cpu_stall_timeout __read_mostly = CONFIG_RCU_CPU_STALL_TIMEOUT;
module_param(rcu_cpu_stall_timeout, int, 0644);
int rcu_exp_cpu_stall_timeout __read_mostly = CONFIG_RCU_EXP_CPU_STALL_TIMEOUT;
module_param(rcu_exp_cpu_stall_timeout, int, 0644);
int rcu_cpu_stall_cputime __read_mostly = IS_ENABLED(CONFIG_RCU_CPU_STALL_CPUTIME);
module_param(rcu_cpu_stall_cputime, int, 0644);
bool rcu_exp_stall_task_details __read_mostly;
module_param(rcu_exp_stall_task_details, bool, 0644);
#endif /* #ifdef CONFIG_RCU_STALL_COMMON */

// Suppress boot-time RCU CPU stall warnings and rcutorture writer stall
// warnings.  Also used by rcutorture even if stall warnings are excluded.
int rcu_cpu_stall_suppress_at_boot __read_mostly; // !0 = suppress boot stalls.
EXPORT_SYMBOL_GPL(rcu_cpu_stall_suppress_at_boot);
module_param(rcu_cpu_stall_suppress_at_boot, int, 0444);

/**
 * get_completed_synchronize_rcu - Return a pre-completed polled state cookie
 *
 * Returns a value that will always be treated by functions like
 * poll_state_synchronize_rcu() as a cookie whose grace period has already
 * completed.
 */
unsigned long get_completed_synchronize_rcu(void)
{
	return RCU_GET_STATE_COMPLETED;
}
EXPORT_SYMBOL_GPL(get_completed_synchronize_rcu);

#ifdef CONFIG_PROVE_RCU

/*
 * Early boot self test parameters.
 */
static bool rcu_self_test;
module_param(rcu_self_test, bool, 0444);

static int rcu_self_test_counter;

static void test_callback(struct rcu_head *r)
{
	rcu_self_test_counter++;
	pr_info("RCU test callback executed %d\n", rcu_self_test_counter);
}

DEFINE_STATIC_SRCU(early_srcu);
static unsigned long early_srcu_cookie;

struct early_boot_kfree_rcu {
	struct rcu_head rh;
};

static void early_boot_test_call_rcu(void)
{
	static struct rcu_head head;
	int idx;
	static struct rcu_head shead;
	struct early_boot_kfree_rcu *rhp;

	idx = srcu_down_read(&early_srcu);
	srcu_up_read(&early_srcu, idx);
	call_rcu(&head, test_callback);
	early_srcu_cookie = start_poll_synchronize_srcu(&early_srcu);
	call_srcu(&early_srcu, &shead, test_callback);
	rhp = kmalloc_obj(*rhp);
	if (!WARN_ON_ONCE(!rhp))
		kfree_rcu(rhp, rh);
}

void rcu_early_boot_tests(void)
{
	pr_info("Running RCU self tests\n");

	if (rcu_self_test)
		early_boot_test_call_rcu();
	rcu_test_sync_prims();
}

static int rcu_verify_early_boot_tests(void)
{
	int ret = 0;
	int early_boot_test_counter = 0;

	if (rcu_self_test) {
		early_boot_test_counter++;
		rcu_barrier();
		early_boot_test_counter++;
		srcu_barrier(&early_srcu);
		WARN_ON_ONCE(!poll_state_synchronize_srcu(&early_srcu, early_srcu_cookie));
		cleanup_srcu_struct(&early_srcu);
	}
	if (rcu_self_test_counter != early_boot_test_counter) {
		WARN_ON(1);
		ret = -1;
	}

	return ret;
}
late_initcall(rcu_verify_early_boot_tests);
#else
void rcu_early_boot_tests(void) {}
#endif /* CONFIG_PROVE_RCU */

#include "tasks.h"

#ifndef CONFIG_TINY_RCU

/*
 * Print any significant non-default boot-time settings.
 */
void __init rcupdate_announce_bootup_oddness(void)
{
	if (rcu_normal)
		pr_info("\tNo expedited grace period (rcu_normal).\n");
	else if (rcu_normal_after_boot)
		pr_info("\tNo expedited grace period (rcu_normal_after_boot).\n");
	else if (rcu_expedited)
		pr_info("\tAll grace periods are expedited (rcu_expedited).\n");
	if (rcu_cpu_stall_suppress)
		pr_info("\tRCU CPU stall warnings suppressed (rcu_cpu_stall_suppress).\n");
	if (rcu_cpu_stall_timeout != CONFIG_RCU_CPU_STALL_TIMEOUT)
		pr_info("\tRCU CPU stall warnings timeout set to %d (rcu_cpu_stall_timeout).\n", rcu_cpu_stall_timeout);
	rcu_tasks_bootup_oddness();
}

#endif /* #ifndef CONFIG_TINY_RCU */
