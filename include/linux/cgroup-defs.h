/* SPDX-License-Identifier: GPL-2.0 */
/*
 * linux/cgroup-defs.h - basic definitions for cgroup
 *
 * This file provides basic type and interface.  Include this file directly
 * only if necessary to avoid cyclic dependencies.
 */
#ifndef _LINUX_CGROUP_DEFS_H
#define _LINUX_CGROUP_DEFS_H

#include <linux/limits.h>
#include <linux/list.h>
#include <linux/idr.h>
#include <linux/wait.h>
#include <linux/mutex.h>
#include <linux/rcupdate.h>
#include <linux/refcount.h>
#include <linux/percpu-refcount.h>
#include <linux/percpu-rwsem.h>
#include <linux/sched.h>
#include <linux/u64_stats_sync.h>
#include <linux/workqueue.h>
#include <linux/psi_types.h>


#define CGROUP_SUBSYS_COUNT 0

static inline void cgroup_threadgroup_change_begin(struct task_struct *tsk)
{
	might_sleep();
}

static inline void cgroup_threadgroup_change_end(struct task_struct *tsk) {}



#endif	/* _LINUX_CGROUP_DEFS_H */
