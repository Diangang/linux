/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_X86_PGALLOC_H
#define _ASM_X86_PGALLOC_H

#include <linux/threads.h>
#include <linux/mm.h>		/* for struct page */
#include <linux/pagemap.h>

#include <asm/cpufeature.h>

#define __HAVE_ARCH_PTE_ALLOC_ONE
#define __HAVE_ARCH_PGD_FREE
#include <asm-generic/pgalloc.h>

/*
 * In case of Page Table Isolation active, we acquire two PGDs instead of one.
 * Being order-1, it is both 8k in size and 8k-aligned.  That lets us just
 * flip bit 12 in a pointer to swap between the two 4k halves.
 */
static inline unsigned int pgd_allocation_order(void)
{
	if (cpu_feature_enabled(X86_FEATURE_PTI))
		return 1;
	return 0;
}

/*
 * Allocate and free page tables.
 */
extern pgd_t *pgd_alloc(struct mm_struct *);
extern void pgd_free(struct mm_struct *mm, pgd_t *pgd);

extern pgtable_t pte_alloc_one(struct mm_struct *);

extern void ___pte_free_tlb(struct mmu_gather *tlb, struct page *pte);

static inline void __pte_free_tlb(struct mmu_gather *tlb, struct page *pte,
				  unsigned long address)
{
	___pte_free_tlb(tlb, pte);
}

static inline void pmd_populate_kernel(struct mm_struct *mm,
				       pmd_t *pmd, pte_t *pte)
{
	set_pmd(pmd, __pmd(__pa(pte) | _PAGE_TABLE));
}

static inline void pmd_populate_kernel_safe(struct mm_struct *mm,
				       pmd_t *pmd, pte_t *pte)
{
	set_pmd_safe(pmd, __pmd(__pa(pte) | _PAGE_TABLE));
}

static inline void pmd_populate(struct mm_struct *mm, pmd_t *pmd,
				struct page *pte)
{
	unsigned long pfn = page_to_pfn(pte);

	set_pmd(pmd, __pmd(((pteval_t)pfn << PAGE_SHIFT) | _PAGE_TABLE));
}

#if CONFIG_PGTABLE_LEVELS > 2
extern void ___pmd_free_tlb(struct mmu_gather *tlb, pmd_t *pmd);

static inline void __pmd_free_tlb(struct mmu_gather *tlb, pmd_t *pmd,
				  unsigned long address)
{
	___pmd_free_tlb(tlb, pmd);
}

static inline void pud_populate(struct mm_struct *mm, pud_t *pud, pmd_t *pmd)
{
	set_pud(pud, __pud(_PAGE_TABLE | __pa(pmd)));
}

static inline void pud_populate_safe(struct mm_struct *mm, pud_t *pud, pmd_t *pmd)
{
	set_pud_safe(pud, __pud(_PAGE_TABLE | __pa(pmd)));
}

#if CONFIG_PGTABLE_LEVELS > 3
static inline void p4d_populate(struct mm_struct *mm, p4d_t *p4d, pud_t *pud)
{
	set_p4d(p4d, __p4d(_PAGE_TABLE | __pa(pud)));
}

static inline void p4d_populate_safe(struct mm_struct *mm, p4d_t *p4d, pud_t *pud)
{
	set_p4d_safe(p4d, __p4d(_PAGE_TABLE | __pa(pud)));
}

extern void ___pud_free_tlb(struct mmu_gather *tlb, pud_t *pud);

static inline void __pud_free_tlb(struct mmu_gather *tlb, pud_t *pud,
				  unsigned long address)
{
	___pud_free_tlb(tlb, pud);
}

#if CONFIG_PGTABLE_LEVELS > 4
static inline void pgd_populate(struct mm_struct *mm, pgd_t *pgd, p4d_t *p4d)
{
	if (!pgtable_l5_enabled())
		return;
	set_pgd(pgd, __pgd(_PAGE_TABLE | __pa(p4d)));
}

static inline void pgd_populate_safe(struct mm_struct *mm, pgd_t *pgd, p4d_t *p4d)
{
	if (!pgtable_l5_enabled())
		return;
	set_pgd_safe(pgd, __pgd(_PAGE_TABLE | __pa(p4d)));
}

extern void ___p4d_free_tlb(struct mmu_gather *tlb, p4d_t *p4d);

static inline void __p4d_free_tlb(struct mmu_gather *tlb, p4d_t *p4d,
				  unsigned long address)
{
	if (pgtable_l5_enabled())
		___p4d_free_tlb(tlb, p4d);
}

#endif	/* CONFIG_PGTABLE_LEVELS > 4 */
#endif	/* CONFIG_PGTABLE_LEVELS > 3 */
#endif	/* CONFIG_PGTABLE_LEVELS > 2 */

#endif /* _ASM_X86_PGALLOC_H */
