/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_PSI_H
#define _LINUX_PSI_H

#include <linux/jump_label.h>
#include <linux/psi_types.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include <linux/cgroup-defs.h>
#include <linux/cgroup.h>

struct seq_file;
struct css_set;


static inline void psi_init(void) {}

static inline void psi_memstall_enter(unsigned long *flags) {}
static inline void psi_memstall_leave(unsigned long *flags) {}

#ifdef CONFIG_CGROUPS
static inline int psi_cgroup_alloc(struct cgroup *cgrp)
{
	return 0;
}
static inline void psi_cgroup_free(struct cgroup *cgrp)
{
}
static inline void cgroup_move_task(struct task_struct *p, struct css_set *to)
{
	rcu_assign_pointer(p->cgroups, to);
}
static inline void psi_cgroup_restart(struct psi_group *group) {}
#endif


#endif /* _LINUX_PSI_H */
