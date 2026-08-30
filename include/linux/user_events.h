/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2022, Microsoft Corporation.
 *
 * Authors:
 *   Beau Belgrave <beaub@linux.microsoft.com>
 */

#ifndef _LINUX_USER_EVENTS_H
#define _LINUX_USER_EVENTS_H

#include <linux/list.h>
#include <linux/refcount.h>
#include <linux/mm_types.h>
#include <linux/workqueue.h>
#include <uapi/linux/user_events.h>

static inline void user_events_fork(struct task_struct *t,
				    u64 clone_flags)
{
}

static inline void user_events_execve(struct task_struct *t)
{
}

static inline void user_events_exit(struct task_struct *t)
{
}

#endif /* _LINUX_USER_EVENTS_H */
