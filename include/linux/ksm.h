/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __LINUX_KSM_H
#define __LINUX_KSM_H
/*
 * Memory merging support.
 *
 * This code enables dynamic sharing of identical pages found in different
 * memory areas, even if they are not shared by fork().
 */

#include <linux/bitops.h>
#include <linux/mm.h>
#include <linux/pagemap.h>
#include <linux/rmap.h>
#include <linux/sched.h>


static inline vma_flags_t ksm_vma_flags(struct mm_struct *mm,
		const struct file *file, vma_flags_t vma_flags)
{
	return vma_flags;
}

static inline int ksm_disable(struct mm_struct *mm)
{
	return 0;
}

static inline void ksm_fork(struct mm_struct *mm, struct mm_struct *oldmm)
{
}

static inline int ksm_execve(struct mm_struct *mm)
{
	return 0;
}

static inline void ksm_exit(struct mm_struct *mm)
{
}

static inline void ksm_might_unmap_zero_page(struct mm_struct *mm, pte_t pte)
{
}

static inline void collect_procs_ksm(const struct folio *folio,
		const struct page *page, struct list_head *to_kill,
		int force_early)
{
}

#ifdef CONFIG_MMU
static inline int ksm_madvise(struct vm_area_struct *vma, unsigned long start,
		unsigned long end, int advice, vm_flags_t *vm_flags)
{
	return 0;
}

static inline struct folio *ksm_might_need_to_copy(struct folio *folio,
			struct vm_area_struct *vma, unsigned long addr)
{
	return folio;
}

static inline void rmap_walk_ksm(struct folio *folio,
			struct rmap_walk_control *rwc)
{
}

static inline void folio_migrate_ksm(struct folio *newfolio, struct folio *old)
{
}
#endif /* CONFIG_MMU */

#endif /* __LINUX_KSM_H */
