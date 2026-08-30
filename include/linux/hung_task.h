/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Detect Hung Task: detecting tasks stuck in D state
 *
 * Copyright (C) 2025 Tongcheng Travel (www.ly.com)
 * Author: Lance Yang <mingzhe.yang@ly.com>
 */
#ifndef __LINUX_HUNG_TASK_H
#define __LINUX_HUNG_TASK_H

#include <linux/bug.h>
#include <linux/sched.h>
#include <linux/compiler.h>

/*
 * @blocker: Combines lock address and blocking type.
 *
 * Since lock pointers are at least 4-byte aligned(32-bit) or 8-byte
 * aligned(64-bit). This leaves the 2 least bits (LSBs) of the pointer
 * always zero. So we can use these bits to encode the specific blocking
 * type.
 *
 * Note that on architectures where this is not guaranteed, or for any
 * unaligned lock, this tracking mechanism is silently skipped for that
 * lock.
 *
 * Type encoding:
 * 00 - Blocked on mutex			(BLOCKER_TYPE_MUTEX)
 * 01 - Blocked on semaphore			(BLOCKER_TYPE_SEM)
 * 10 - Blocked on rw-semaphore as READER	(BLOCKER_TYPE_RWSEM_READER)
 * 11 - Blocked on rw-semaphore as WRITER	(BLOCKER_TYPE_RWSEM_WRITER)
 */
#define BLOCKER_TYPE_MUTEX		0x00UL
#define BLOCKER_TYPE_SEM		0x01UL
#define BLOCKER_TYPE_RWSEM_READER	0x02UL
#define BLOCKER_TYPE_RWSEM_WRITER	0x03UL

#define BLOCKER_TYPE_MASK		0x03UL

static inline void hung_task_set_blocker(void *lock, unsigned long type)
{
}
static inline void hung_task_clear_blocker(void)
{
}
static inline unsigned long hung_task_get_blocker_type(unsigned long blocker)
{
	return 0UL;
}
static inline void *hung_task_blocker_to_lock(unsigned long blocker)
{
	return NULL;
}

#endif /* __LINUX_HUNG_TASK_H */
