/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_HUGE_MM_H
#define _LINUX_HUGE_MM_H

#include <linux/mm_types.h>
#include <linux/pgtable.h>

#include <linux/fs.h> /* only for vma_is_dax() */

int copy_huge_pmd(struct mm_struct *dst_mm, struct mm_struct *src_mm,
		  pmd_t *dst_pmd, pmd_t *src_pmd, unsigned long addr,
		  struct vm_area_struct *dst_vma, struct vm_area_struct *src_vma);
int copy_huge_pud(struct mm_struct *dst_mm, struct mm_struct *src_mm,
		  pud_t *dst_pud, pud_t *src_pud, unsigned long addr,
		  struct vm_area_struct *vma);

bool zap_huge_pmd(struct mmu_gather *tlb, struct vm_area_struct *vma, pmd_t *pmd,
		  unsigned long addr);
int zap_huge_pud(struct mmu_gather *tlb, struct vm_area_struct *vma, pud_t *pud,
		 unsigned long addr);
int change_huge_pmd(struct mmu_gather *tlb, struct vm_area_struct *vma,
		    pmd_t *pmd, unsigned long addr, pgprot_t newprot,
		    unsigned long cp_flags);

/*
 * Mask of all large folio orders supported for anonymous THP; all orders up to
 * and including PMD_ORDER, except order-0 (which is not "huge") and order-1
 * (which is a limitation of the THP implementation).
 */
#define THP_ORDERS_ALL_ANON	((BIT(PMD_ORDER + 1) - 1) & ~(BIT(0) | BIT(1)))

/*
 * Mask of all large folio orders supported for file THP. Folios in a DAX
 * file is never split and the MAX_PAGECACHE_ORDER limit does not apply to
 * it.  Same to PFNMAPs where there's neither page* nor pagecache.
 */
#define THP_ORDERS_ALL_SPECIAL_DAX	\
	(BIT(PMD_ORDER) | BIT(PUD_ORDER))
#define THP_ORDERS_ALL_FILE_DEFAULT	\
	((BIT(MAX_PAGECACHE_ORDER + 1) - 1) & ~BIT(0))

/*
 * Mask of all large folio orders supported for THP.
 */
#define THP_ORDERS_ALL	\
	(THP_ORDERS_ALL_ANON | THP_ORDERS_ALL_SPECIAL_DAX | THP_ORDERS_ALL_FILE_DEFAULT)

enum tva_type {
	TVA_SMAPS,		/* Exposing "THPeligible:" in smaps. */
	TVA_PAGEFAULT,		/* Serving a page fault. */
};

#define thp_vma_allowable_order(vma, vm_flags, type, order) \
	(!!thp_vma_allowable_orders(vma, vm_flags, type, BIT(order)))

#define split_folio(f) split_folio_to_list(f, NULL)

#define HPAGE_PMD_SHIFT ({ BUILD_BUG(); 0; })
#define HPAGE_PUD_SHIFT ({ BUILD_BUG(); 0; })

#define HPAGE_PMD_ORDER (HPAGE_PMD_SHIFT-PAGE_SHIFT)
#define HPAGE_PMD_NR (1<<HPAGE_PMD_ORDER)
#define HPAGE_PMD_MASK	(~(HPAGE_PMD_SIZE - 1))
#define HPAGE_PMD_SIZE	((1UL) << HPAGE_PMD_SHIFT)

#define HPAGE_PUD_ORDER (HPAGE_PUD_SHIFT-PAGE_SHIFT)
#define HPAGE_PUD_NR (1<<HPAGE_PUD_ORDER)
#define HPAGE_PUD_MASK	(~(HPAGE_PUD_SIZE - 1))
#define HPAGE_PUD_SIZE	((1UL) << HPAGE_PUD_SHIFT)

enum mthp_stat_item {
	MTHP_STAT_ANON_FAULT_ALLOC,
	MTHP_STAT_ANON_FAULT_FALLBACK,
	MTHP_STAT_ANON_FAULT_FALLBACK_CHARGE,
	MTHP_STAT_ZSWPOUT,
	MTHP_STAT_SWPIN,
	MTHP_STAT_SWPIN_FALLBACK,
	MTHP_STAT_SWPIN_FALLBACK_CHARGE,
	MTHP_STAT_SWPOUT,
	MTHP_STAT_SWPOUT_FALLBACK,
	MTHP_STAT_SHMEM_ALLOC,
	MTHP_STAT_SHMEM_FALLBACK,
	MTHP_STAT_SHMEM_FALLBACK_CHARGE,
	MTHP_STAT_SPLIT,
	MTHP_STAT_SPLIT_FAILED,
	MTHP_STAT_SPLIT_DEFERRED,
	MTHP_STAT_NR_ANON,
	MTHP_STAT_NR_ANON_PARTIALLY_MAPPED,
	__MTHP_STAT_COUNT
};

static inline void mod_mthp_stat(int order, enum mthp_stat_item item, int delta)
{
}

static inline void count_mthp_stat(int order, enum mthp_stat_item item)
{
}


static inline bool folio_test_pmd_mappable(struct folio *folio)
{
	return false;
}

static inline bool thp_vma_suitable_order(struct vm_area_struct *vma,
		unsigned long addr, int order)
{
	return false;
}

static inline unsigned long thp_vma_suitable_orders(struct vm_area_struct *vma,
		unsigned long addr, unsigned long orders)
{
	return 0;
}

static inline unsigned long thp_vma_allowable_orders(struct vm_area_struct *vma,
					vm_flags_t vm_flags,
					enum tva_type type,
					unsigned long orders)
{
	return 0;
}

#define transparent_hugepage_flags 0UL

#define thp_get_unmapped_area	NULL
static inline int
split_huge_page_to_list_to_order(struct page *page, struct list_head *list,
		unsigned int new_order)
{
	VM_WARN_ON_ONCE_PAGE(1, page);
	return -EINVAL;
}
static inline int split_huge_page_to_order(struct page *page, unsigned int new_order)
{
	VM_WARN_ON_ONCE_PAGE(1, page);
	return -EINVAL;
}
static inline int split_huge_page(struct page *page)
{
	VM_WARN_ON_ONCE_PAGE(1, page);
	return -EINVAL;
}

static inline unsigned int min_order_for_split(struct folio *folio)
{
	VM_WARN_ON_ONCE_FOLIO(1, folio);
	return 0;
}

static inline int split_folio_to_list(struct folio *folio, struct list_head *list)
{
	VM_WARN_ON_ONCE_FOLIO(1, folio);
	return -EINVAL;
}

static inline int try_folio_split_to_order(struct folio *folio,
		struct page *page, unsigned int new_order)
{
	VM_WARN_ON_ONCE_FOLIO(1, folio);
	return -EINVAL;
}

static inline void deferred_split_folio(struct folio *folio, bool partially_mapped) {}
#define split_huge_pmd(__vma, __pmd, __address)	\
	do { } while (0)

static inline void __split_huge_pmd(struct vm_area_struct *vma, pmd_t *pmd,
		unsigned long address, bool freeze) {}
static inline void split_huge_pmd_address(struct vm_area_struct *vma,
		unsigned long address, bool freeze) {}
static inline void split_huge_pmd_locked(struct vm_area_struct *vma,
					 unsigned long address, pmd_t *pmd,
					 bool freeze) {}

static inline bool unmap_huge_pmd_locked(struct vm_area_struct *vma,
					 unsigned long addr, pmd_t *pmdp,
					 struct folio *folio)
{
	return false;
}

#define split_huge_pud(__vma, __pmd, __address)	\
	do { } while (0)

static inline void vma_adjust_trans_huge(struct vm_area_struct *vma,
					 unsigned long start,
					 unsigned long end,
					 struct vm_area_struct *next)
{
}
static inline spinlock_t *pmd_trans_huge_lock(pmd_t *pmd,
		struct vm_area_struct *vma)
{
	return NULL;
}
static inline spinlock_t *pud_trans_huge_lock(pud_t *pud,
		struct vm_area_struct *vma)
{
	return NULL;
}

static inline bool is_huge_zero_folio(const struct folio *folio)
{
	return false;
}

static inline bool is_huge_zero_pfn(unsigned long pfn)
{
	return false;
}

static inline bool is_huge_zero_pmd(pmd_t pmd)
{
	return false;
}

static inline void mm_put_huge_zero_folio(struct mm_struct *mm)
{
	return;
}

static inline bool thp_migration_supported(void)
{
	return false;
}

static inline int highest_order(unsigned long orders)
{
	return 0;
}

static inline int next_order(unsigned long *orders, int prev)
{
	return 0;
}

static inline void __split_huge_pud(struct vm_area_struct *vma, pud_t *pud,
				    unsigned long address)
{
}

static inline int change_huge_pud(struct mmu_gather *tlb,
				  struct vm_area_struct *vma, pud_t *pudp,
				  unsigned long addr, pgprot_t newprot,
				  unsigned long cp_flags)
{
	return 0;
}

static inline struct folio *get_persistent_huge_zero_folio(void)
{
	return NULL;
}

static inline bool pmd_is_huge(pmd_t pmd)
{
	return false;
}

static inline bool is_pmd_order(unsigned int order)
{
	return order == HPAGE_PMD_ORDER;
}

static inline int split_folio_to_list_to_order(struct folio *folio,
		struct list_head *list, int new_order)
{
	return split_huge_page_to_list_to_order(&folio->page, list, new_order);
}

static inline int split_folio_to_order(struct folio *folio, int new_order)
{
	return split_folio_to_list_to_order(folio, NULL, new_order);
}

/**
 * largest_zero_folio - Get the largest zero size folio available
 *
 * This function shall be used when mm_get_huge_zero_folio() cannot be
 * used as there is no appropriate mm lifetime to tie the huge zero folio
 * from the caller.
 *
 * Deduce the size of the folio with folio_size instead of assuming the
 * folio size.
 *
 * Return: pointer to PMD sized zero folio if CONFIG_PERSISTENT_HUGE_ZERO_FOLIO
 * is enabled or a single page sized zero folio
 */
static inline struct folio *largest_zero_folio(void)
{
	struct folio *folio = get_persistent_huge_zero_folio();

	if (folio)
		return folio;

	return page_folio(ZERO_PAGE(0));
}
#endif /* _LINUX_HUGE_MM_H */
