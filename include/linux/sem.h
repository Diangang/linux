/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_SEM_H
#define _LINUX_SEM_H

#include <uapi/linux/sem.h>
#include <linux/sem_types.h>

struct task_struct;


static inline int copy_semundo(u64 clone_flags, struct task_struct *tsk)
{
	return 0;
}

static inline void exit_sem(struct task_struct *tsk)
{
	return;
}

#endif /* _LINUX_SEM_H */
