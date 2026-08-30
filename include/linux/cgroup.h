/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_CGROUP_H
#define _LINUX_CGROUP_H
/*
 *  cgroup interface
 *
 *  Copyright (C) 2003 BULL SA
 *  Copyright (C) 2004-2006 Silicon Graphics, Inc.
 *
 */

#include <linux/sched.h>
#include <linux/nodemask.h>
#include <linux/list.h>
#include <linux/rculist.h>
#include <linux/cgroupstats.h>
#include <linux/fs.h>
#include <linux/seq_file.h>
#include <linux/kernfs.h>
#include <linux/jump_label.h>
#include <linux/types.h>
#include <linux/notifier.h>
#include <linux/ns_common.h>
#include <linux/nsproxy.h>
#include <linux/user_namespace.h>
#include <linux/refcount.h>
#include <linux/kernel_stat.h>

#include <linux/cgroup-defs.h>
#include <linux/cgroup_namespace.h>

struct kernel_clone_args;

/*
 * All weight knobs on the default hierarchy should use the following min,
 * default and max values.  The default value is the logarithmic center of
 * MIN and MAX and allows 100x to be expressed in both directions.
 */
#define CGROUP_WEIGHT_MIN		1
#define CGROUP_WEIGHT_DFL		100
#define CGROUP_WEIGHT_MAX		10000


struct cgroup_subsys_state;
struct cgroup;

static inline u64 cgroup_id(const struct cgroup *cgrp) { return 1; }
static inline void css_get(struct cgroup_subsys_state *css) {}
static inline void css_put(struct cgroup_subsys_state *css) {}
static inline void cgroup_lock(void) {}
static inline void cgroup_unlock(void) {}
static inline int cgroup_attach_task_all(struct task_struct *from,
					 struct task_struct *t) { return 0; }
static inline int cgroupstats_build(struct cgroupstats *stats,
				    struct dentry *dentry) { return -EINVAL; }

static inline void cgroup_fork(struct task_struct *p) {}
static inline int cgroup_can_fork(struct task_struct *p,
				  struct kernel_clone_args *kargs) { return 0; }
static inline void cgroup_cancel_fork(struct task_struct *p,
				      struct kernel_clone_args *kargs) {}
static inline void cgroup_post_fork(struct task_struct *p,
				    struct kernel_clone_args *kargs) {}
static inline void cgroup_task_exit(struct task_struct *p) {}
static inline void cgroup_task_dead(struct task_struct *p) {}
static inline void cgroup_task_release(struct task_struct *p) {}
static inline void cgroup_task_free(struct task_struct *p) {}

static inline int cgroup_init_early(void) { return 0; }
static inline int cgroup_init(void) { return 0; }
static inline void cgroup_init_kthreadd(void) {}
static inline void cgroup_kthread_ready(void) {}

static inline struct cgroup *cgroup_parent(struct cgroup *cgrp)
{
	return NULL;
}

static inline bool cgroup_psi_enabled(void)
{
	return false;
}

static inline bool task_under_cgroup_hierarchy(struct task_struct *task,
					       struct cgroup *ancestor)
{
	return true;
}

static inline void cgroup_path_from_kernfs_id(u64 id, char *buf, size_t buflen)
{}


static inline void cgroup_account_cputime(struct task_struct *task,
					  u64 delta_exec) {}
static inline void cgroup_account_cputime_field(struct task_struct *task,
						enum cpu_usage_stat index,
						u64 delta_exec) {}



static inline void cgroup_enter_frozen(void) { }
static inline void cgroup_leave_frozen(bool always_leave) { }
static inline bool cgroup_task_frozen(struct task_struct *task)
{
	return false;
}


struct cgroup *task_get_cgroup1(struct task_struct *tsk, int hierarchy_id);

struct cgroup_of_peak *of_peak(struct kernfs_open_file *of);

#endif /* _LINUX_CGROUP_H */
