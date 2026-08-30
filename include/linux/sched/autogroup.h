/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_SCHED_AUTOGROUP_H
#define _LINUX_SCHED_AUTOGROUP_H

struct signal_struct;
struct task_struct;
struct task_group;
struct seq_file;

static inline void sched_autogroup_create_attach(struct task_struct *p) { }
static inline void sched_autogroup_detach(struct task_struct *p) { }
static inline void sched_autogroup_fork(struct signal_struct *sig) { }
static inline void sched_autogroup_exit(struct signal_struct *sig) { }
static inline void sched_autogroup_exit_task(struct task_struct *p) { }


#endif /* _LINUX_SCHED_AUTOGROUP_H */
