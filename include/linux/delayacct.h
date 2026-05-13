/* SPDX-License-Identifier: GPL-2.0-or-later */
/* delayacct.h - per-task delay accounting
 *
 * Copyright (C) Shailabh Nagar, IBM Corp. 2006
 */

#ifndef _LINUX_DELAYACCT_H
#define _LINUX_DELAYACCT_H

#include <uapi/linux/taskstats.h>


#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/jump_label.h>

static inline void delayacct_init(void)
{}
static inline void delayacct_tsk_init(struct task_struct *tsk)
{}
static inline void delayacct_tsk_free(struct task_struct *tsk)
{}
static inline void delayacct_blkio_start(void)
{}
static inline void delayacct_blkio_end(struct task_struct *p)
{}
static inline int delayacct_add_tsk(struct taskstats *d,
					struct task_struct *tsk)
{ return 0; }
static inline __u64 delayacct_blkio_ticks(struct task_struct *tsk)
{ return 0; }
static inline int delayacct_is_task_waiting_on_io(struct task_struct *p)
{ return 0; }
static inline void delayacct_freepages_start(void)
{}
static inline void delayacct_freepages_end(void)
{}
static inline void delayacct_thrashing_start(bool *in_thrashing)
{}
static inline void delayacct_thrashing_end(bool *in_thrashing)
{}
static inline void delayacct_swapin_start(void)
{}
static inline void delayacct_swapin_end(void)
{}
static inline void delayacct_compact_start(void)
{}
static inline void delayacct_compact_end(void)
{}
static inline void delayacct_wpcopy_start(void)
{}
static inline void delayacct_wpcopy_end(void)
{}
static inline void delayacct_irq(struct task_struct *task, u32 delta)
{}


#endif
