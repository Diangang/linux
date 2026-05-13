/* SPDX-License-Identifier: GPL-2.0 */
/* taskstats_kern.h - kernel header for per-task statistics interface
 *
 * Copyright (C) Shailabh Nagar, IBM Corp. 2006
 *           (C) Balbir Singh,   IBM Corp. 2006
 */

#ifndef _LINUX_TASKSTATS_KERN_H
#define _LINUX_TASKSTATS_KERN_H

#include <linux/taskstats.h>
#include <linux/sched/signal.h>
#include <linux/slab.h>

static inline void taskstats_exit(struct task_struct *tsk, int group_dead)
{}
static inline void taskstats_tgid_free(struct signal_struct *sig)
{}
static inline void taskstats_init_early(void)
{}

#endif

