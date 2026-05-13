/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Read-Copy Update mechanism for mutual exclusion (tree-based version)
 * Internal non-public definitions that provide either classic
 * or preemptible semantics.
 *
 * Copyright Red Hat, 2009
 * Copyright IBM Corporation, 2009
 * Copyright SUSE, 2021
 *
 * Author: Ingo Molnar <mingo@elte.hu>
 *	   Paul E. McKenney <paulmck@linux.ibm.com>
 *	   Frederic Weisbecker <frederic@kernel.org>
 */


/* No ->nocb_lock to acquire.  */
static void rcu_nocb_lock(struct rcu_data *rdp)
{
}

/* No ->nocb_lock to release.  */
static void rcu_nocb_unlock(struct rcu_data *rdp)
{
}

/* No ->nocb_lock to release.  */
static void rcu_nocb_unlock_irqrestore(struct rcu_data *rdp,
				       unsigned long flags)
{
	local_irq_restore(flags);
}

/* Lockdep check that ->cblist may be safely accessed. */
static void rcu_lockdep_assert_cblist_protected(struct rcu_data *rdp)
{
	lockdep_assert_irqs_disabled();
}

static void rcu_nocb_gp_cleanup(struct swait_queue_head *sq)
{
}

static struct swait_queue_head *rcu_nocb_gp_get(struct rcu_node *rnp)
{
	return NULL;
}

static void rcu_init_one_nocb(struct rcu_node *rnp)
{
}

static bool wake_nocb_gp(struct rcu_data *rdp)
{
	return false;
}

static bool rcu_nocb_flush_bypass(struct rcu_data *rdp, struct rcu_head *rhp,
				  unsigned long j, bool lazy)
{
	return true;
}

static void call_rcu_nocb(struct rcu_data *rdp, struct rcu_head *head,
			  rcu_callback_t func, unsigned long flags, bool lazy)
{
	WARN_ON_ONCE(1);  /* Should be dead code! */
}

static void __call_rcu_nocb_wake(struct rcu_data *rdp, bool was_empty,
				 unsigned long flags)
{
	WARN_ON_ONCE(1);  /* Should be dead code! */
}

static void __init rcu_boot_init_nocb_percpu_data(struct rcu_data *rdp)
{
}

static int rcu_nocb_need_deferred_wakeup(struct rcu_data *rdp, int level)
{
	return false;
}

static bool do_nocb_deferred_wakeup(struct rcu_data *rdp)
{
	return false;
}

static void rcu_spawn_cpu_nocb_kthread(int cpu)
{
}

static void show_rcu_nocb_state(struct rcu_data *rdp)
{
}
