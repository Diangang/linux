/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_KHUGEPAGED_H
#define _LINUX_KHUGEPAGED_H

#include <linux/mm.h>

extern unsigned int khugepaged_max_ptes_none __read_mostly;
static inline void khugepaged_fork(struct mm_struct *mm, struct mm_struct *oldmm)
{
}
static inline void khugepaged_exit(struct mm_struct *mm)
{
}
static inline void khugepaged_enter_vma(struct vm_area_struct *vma,
					vm_flags_t vm_flags)
{
}
static inline void collapse_pte_mapped_thp(struct mm_struct *mm,
		unsigned long addr, bool install_pmd)
{
}

static inline void khugepaged_min_free_kbytes_update(void)
{
}

static inline bool current_is_khugepaged(void)
{
	return false;
}

#endif /* _LINUX_KHUGEPAGED_H */
