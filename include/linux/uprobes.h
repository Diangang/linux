/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef _LINUX_UPROBES_H
#define _LINUX_UPROBES_H
/*
 * User-space Probes (UProbes)
 *
 * Copyright (C) IBM Corporation, 2008-2012
 * Authors:
 *	Srikar Dronamraju
 *	Jim Keniston
 * Copyright (C) 2011-2012 Red Hat, Inc., Peter Zijlstra
 */

#include <linux/errno.h>
#include <linux/rbtree.h>
#include <linux/types.h>
#include <linux/wait.h>
#include <linux/timer.h>
#include <linux/seqlock.h>
#include <linux/mutex.h>

struct uprobe;
struct vm_area_struct;
struct mm_struct;
struct inode;
struct notifier_block;
struct page;

/*
 * Allowed return values from uprobe consumer's handler callback
 * with following meaning:
 *
 * UPROBE_HANDLER_REMOVE
 * - Remove the uprobe breakpoint from current->mm.
 * UPROBE_HANDLER_IGNORE
 * - Ignore ret_handler callback for this consumer.
 */
#define UPROBE_HANDLER_REMOVE		1
#define UPROBE_HANDLER_IGNORE		2

#define MAX_URETPROBE_DEPTH		64

#define UPROBE_NO_TRAMPOLINE_VADDR	(~0UL)

struct uprobe_consumer {
	/*
	 * handler() can return UPROBE_HANDLER_REMOVE to signal the need to
	 * unregister uprobe for current process. If UPROBE_HANDLER_REMOVE is
	 * returned, filter() callback has to be implemented as well and it
	 * should return false to "confirm" the decision to uninstall uprobe
	 * for the current process. If filter() is omitted or returns true,
	 * UPROBE_HANDLER_REMOVE is effectively ignored.
	 */
	int (*handler)(struct uprobe_consumer *self, struct pt_regs *regs, __u64 *data);
	int (*ret_handler)(struct uprobe_consumer *self,
				unsigned long func,
				struct pt_regs *regs, __u64 *data);
	bool (*filter)(struct uprobe_consumer *self, struct mm_struct *mm);

	struct list_head cons_node;

	__u64 id;	/* set when uprobe_consumer is registered */
};

struct uprobes_state {
};

static inline void uprobes_init(void)
{
}

#define uprobe_get_trap_addr(regs)	instruction_pointer(regs)

static inline struct uprobe *
uprobe_register(struct inode *inode, loff_t offset, loff_t ref_ctr_offset, struct uprobe_consumer *uc)
{
	return ERR_PTR(-ENOSYS);
}
static inline int
uprobe_apply(struct uprobe* uprobe, struct uprobe_consumer *uc, bool add)
{
	return -ENOSYS;
}
static inline void
uprobe_unregister_nosync(struct uprobe *uprobe, struct uprobe_consumer *uc)
{
}
static inline void uprobe_unregister_sync(void)
{
}
static inline int uprobe_mmap(struct vm_area_struct *vma)
{
	return 0;
}
static inline void
uprobe_munmap(struct vm_area_struct *vma, unsigned long start, unsigned long end)
{
}
static inline void uprobe_start_dup_mmap(void)
{
}
static inline void uprobe_end_dup_mmap(void)
{
}
static inline void
uprobe_dup_mmap(struct mm_struct *oldmm, struct mm_struct *newmm)
{
}
static inline void uprobe_notify_resume(struct pt_regs *regs)
{
}
static inline bool uprobe_deny_signal(void)
{
	return false;
}
static inline void uprobe_free_utask(struct task_struct *t)
{
}
static inline void uprobe_copy_process(struct task_struct *t, u64 flags)
{
}
static inline void uprobe_clear_state(struct mm_struct *mm)
{
}
#endif	/* _LINUX_UPROBES_H */
