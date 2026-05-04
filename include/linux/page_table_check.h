/* SPDX-License-Identifier: GPL-2.0 */

/*
 * Copyright (c) 2021, Google LLC.
 * Pasha Tatashin <pasha.tatashin@soleen.com>
 */
#ifndef __LINUX_PAGE_TABLE_CHECK_H
#define __LINUX_PAGE_TABLE_CHECK_H


static inline void page_table_check_alloc(struct page *page, unsigned int order)
{
}

static inline void page_table_check_free(struct page *page, unsigned int order)
{
}

static inline void page_table_check_pte_clear(struct mm_struct *mm,
					      unsigned long addr, pte_t pte)
{
}

static inline void page_table_check_pmd_clear(struct mm_struct *mm,
					      unsigned long addr, pmd_t pmd)
{
}

static inline void page_table_check_pud_clear(struct mm_struct *mm,
					      unsigned long addr, pud_t pud)
{
}

static inline void page_table_check_ptes_set(struct mm_struct *mm,
					     unsigned long addr, pte_t *ptep,
					     pte_t pte, unsigned int nr)
{
}

static inline void page_table_check_pmds_set(struct mm_struct *mm,
		unsigned long addr, pmd_t *pmdp, pmd_t pmd, unsigned int nr)
{
}

static inline void page_table_check_puds_set(struct mm_struct *mm,
		unsigned long addr, pud_t *pudp, pud_t pud, unsigned int nr)
{
}

static inline void page_table_check_pte_clear_range(struct mm_struct *mm,
						    unsigned long addr,
						    pmd_t pmd)
{
}


#define page_table_check_pmd_set(mm, addr, pmdp, pmd)	page_table_check_pmds_set(mm, addr, pmdp, pmd, 1)
#define page_table_check_pud_set(mm, addr, pudp, pud)	page_table_check_puds_set(mm, addr, pudp, pud, 1)

#endif /* __LINUX_PAGE_TABLE_CHECK_H */
