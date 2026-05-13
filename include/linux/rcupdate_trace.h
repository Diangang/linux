/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Read-Copy Update mechanism for mutual exclusion, adapted for tracing.
 *
 * Copyright (C) 2020 Paul E. McKenney.
 */

#ifndef __LINUX_RCUPDATE_TRACE_H
#define __LINUX_RCUPDATE_TRACE_H

#include <linux/sched.h>
#include <linux/rcupdate.h>
#include <linux/cleanup.h>



static inline int rcu_read_lock_trace_held(void)
{
	return 1;
}


/*
 * The BPF JIT forms these addresses even when it doesn't call these
 * functions, so provide definitions that result in runtime errors.
 */
static inline void call_rcu_tasks_trace(struct rcu_head *rhp, rcu_callback_t func) { BUG(); }
static inline void rcu_read_lock_trace(void) { BUG(); }
static inline void rcu_read_unlock_trace(void) { BUG(); }

DEFINE_LOCK_GUARD_0(rcu_tasks_trace,
	rcu_read_lock_trace(),
	rcu_read_unlock_trace())

#endif /* __LINUX_RCUPDATE_TRACE_H */
