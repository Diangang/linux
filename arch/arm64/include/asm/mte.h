/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2020 ARM Ltd.
 */
#ifndef __ASM_MTE_H
#define __ASM_MTE_H

#include <asm/compiler.h>
#include <asm/mte-def.h>

#ifndef __ASSEMBLER__

#include <linux/bitfield.h>
#include <linux/kasan-enabled.h>
#include <linux/page-flags.h>
#include <linux/sched.h>
#include <linux/types.h>

#include <asm/pgtable-types.h>

void mte_clear_page_tags(void *addr);
unsigned long mte_copy_tags_from_user(void *to, const void __user *from,
				      unsigned long n);
unsigned long mte_copy_tags_to_user(void __user *to, void *from,
				    unsigned long n);
int mte_save_tags(struct page *page);
void mte_save_page_tags(const void *page_addr, void *tag_storage);
void mte_restore_tags(swp_entry_t entry, struct page *page);
void mte_restore_page_tags(void *page_addr, const void *tag_storage);
void mte_invalidate_tags(int type, pgoff_t offset);
void mte_invalidate_tags_area(int type);
void *mte_allocate_tag_storage(void);
void mte_free_tag_storage(char *storage);

/* unused without MTE support, silence the compiler */
#define PG_mte_tagged	0

static inline void set_page_mte_tagged(struct page *page)
{
}
static inline bool page_mte_tagged(struct page *page)
{
	return false;
}
static inline bool try_page_mte_tagging(struct page *page)
{
	return false;
}
static inline void mte_zero_clear_page_tags(void *addr)
{
}
static inline void mte_sync_tags(pte_t pte, unsigned int nr_pages)
{
}
static inline void mte_copy_page_tags(void *kto, const void *kfrom)
{
}
static inline void mte_thread_init_user(void)
{
}
static inline void mte_thread_switch(struct task_struct *next)
{
}
static inline void mte_suspend_enter(void)
{
}
static inline void mte_suspend_exit(void)
{
}
static inline long set_mte_ctrl(struct task_struct *task, unsigned long arg)
{
	return 0;
}
static inline long get_mte_ctrl(struct task_struct *task)
{
	return 0;
}
static inline int mte_ptrace_copy_tags(struct task_struct *child,
				       long request, unsigned long addr,
				       unsigned long data)
{
	return -EIO;
}

static inline void folio_set_hugetlb_mte_tagged(struct folio *folio)
{
}

static inline bool folio_test_hugetlb_mte_tagged(struct folio *folio)
{
	return false;
}

static inline bool folio_try_hugetlb_mte_tagging(struct folio *folio)
{
	return false;
}

static inline void mte_disable_tco_entry(struct task_struct *task)
{
	if (!system_supports_mte())
		return;

	/*
	 * Re-enable tag checking (TCO set on exception entry). This is only
	 * necessary if MTE is enabled in either the kernel or the userspace
	 * task in synchronous or asymmetric mode (SCTLR_EL1.TCF0 bit 0 is set
	 * for both). With MTE disabled in the kernel and disabled or
	 * asynchronous in userspace, tag check faults (including in uaccesses)
	 * are not reported, therefore there is no need to re-enable checking.
	 * This is beneficial on microarchitectures where re-enabling TCO is
	 * expensive.
	 */
	if (kasan_hw_tags_enabled() ||
	    (task->thread.sctlr_user & (1UL << SCTLR_EL1_TCF0_SHIFT)))
		asm volatile(SET_PSTATE_TCO(0));
}

static inline void mte_check_tfsr_el1(void)
{
}
static inline void mte_check_tfsr_entry(void)
{
}
static inline void mte_check_tfsr_exit(void)
{
}

#endif /* __ASSEMBLER__ */
#endif /* __ASM_MTE_H  */
