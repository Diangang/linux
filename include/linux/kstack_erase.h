/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_KSTACK_ERASE_H
#define _LINUX_KSTACK_ERASE_H

#include <linux/sched.h>
#include <linux/sched/task_stack.h>

/*
 * Check that the poison value points to the unused hole in the
 * virtual memory map for your platform.
 */
#define KSTACK_ERASE_POISON -0xBEEF
#define KSTACK_ERASE_SEARCH_DEPTH 128

static inline void stackleak_task_init(struct task_struct *t) { }

#endif
