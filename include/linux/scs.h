/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Shadow Call Stack support.
 *
 * Copyright (C) 2019 Google LLC
 */

#ifndef _LINUX_SCS_H
#define _LINUX_SCS_H

#include <linux/gfp.h>
#include <linux/poison.h>
#include <linux/sched.h>
#include <linux/sizes.h>


static inline void *scs_alloc(int node) { return NULL; }
static inline void scs_free(void *s) {}
static inline void scs_init(void) {}
static inline void scs_task_reset(struct task_struct *tsk) {}
static inline int scs_prepare(struct task_struct *tsk, int node) { return 0; }
static inline void scs_release(struct task_struct *tsk) {}
static inline bool task_scs_end_corrupted(struct task_struct *tsk) { return false; }
static inline bool scs_is_enabled(void) { return false; }
static inline bool scs_is_dynamic(void) { return false; }


#endif /* _LINUX_SCS_H */
