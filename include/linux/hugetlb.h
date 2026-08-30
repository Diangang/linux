/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_HUGETLB_H
#define _LINUX_HUGETLB_H

#include <linux/mm.h>
#include <linux/mm_types.h>
#include <linux/mmdebug.h>
#include <linux/fs.h>
#include <linux/hugetlb_inline.h>
#include <linux/cgroup.h>
#include <linux/page_ref.h>
#include <linux/list.h>
#include <linux/kref.h>
#include <linux/pgtable.h>
#include <linux/gfp.h>
#include <linux/userfaultfd_k.h>
#include <linux/nodemask.h>

struct mmu_gather;
struct node;

void free_huge_folio(struct folio *folio);


static inline void hugetlb_dup_vma_private(struct vm_area_struct *vma)
{
}

static inline void clear_vma_resv_huge_pages(struct vm_area_struct *vma)
{
}

static inline unsigned long hugetlb_total_pages(void)
{
	return 0;
}

static inline struct address_space *hugetlb_folio_mapping_lock_write(
							struct folio *folio)
{
	return NULL;
}

static inline int huge_pmd_unshare(struct mmu_gather *tlb,
		struct vm_area_struct *vma, unsigned long addr, pte_t *ptep)
{
	return 0;
}

static inline void huge_pmd_unshare_flush(struct mmu_gather *tlb,
		struct vm_area_struct *vma)
{
}

static inline void adjust_range_if_pmd_sharing_possible(
				struct vm_area_struct *vma,
				unsigned long *start, unsigned long *end)
{
}

static inline void hugetlb_zap_begin(
				struct vm_area_struct *vma,
				unsigned long *start, unsigned long *end)
{
}

static inline void hugetlb_zap_end(
				struct vm_area_struct *vma,
				struct zap_details *details)
{
}

static inline int copy_hugetlb_page_range(struct mm_struct *dst,
					  struct mm_struct *src,
					  struct vm_area_struct *dst_vma,
					  struct vm_area_struct *src_vma)
{
	BUG();
	return 0;
}

static inline int move_hugetlb_page_tables(struct vm_area_struct *vma,
					   struct vm_area_struct *new_vma,
					   unsigned long old_addr,
					   unsigned long new_addr,
					   unsigned long len)
{
	BUG();
	return 0;
}

static inline void hugetlb_report_meminfo(struct seq_file *m)
{
}

static inline int hugetlb_report_node_meminfo(char *buf, int len, int nid)
{
	return 0;
}

static inline void hugetlb_show_meminfo_node(int nid)
{
}

static inline void hugetlb_vma_lock_read(struct vm_area_struct *vma)
{
}

static inline void hugetlb_vma_unlock_read(struct vm_area_struct *vma)
{
}

static inline void hugetlb_vma_lock_write(struct vm_area_struct *vma)
{
}

static inline void hugetlb_vma_unlock_write(struct vm_area_struct *vma)
{
}

static inline int hugetlb_vma_trylock_write(struct vm_area_struct *vma)
{
	return 1;
}

static inline void hugetlb_vma_assert_locked(struct vm_area_struct *vma)
{
}

static inline int is_hugepage_only_range(struct mm_struct *mm,
					unsigned long addr, unsigned long len)
{
	return 0;
}


static inline pte_t *huge_pte_offset(struct mm_struct *mm, unsigned long addr,
					unsigned long sz)
{
	return NULL;
}

static inline bool folio_isolate_hugetlb(struct folio *folio, struct list_head *list)
{
	return false;
}

static inline int get_hwpoison_hugetlb_folio(struct folio *folio, bool *hugetlb, bool unpoison)
{
	return 0;
}

static inline int get_huge_page_for_hwpoison(unsigned long pfn, int flags,
					bool *migratable_cleared)
{
	return 0;
}

static inline void folio_putback_hugetlb(struct folio *folio)
{
}

static inline void move_hugetlb_state(struct folio *old_folio,
					struct folio *new_folio, int reason)
{
}

static inline long hugetlb_change_protection(
			struct vm_area_struct *vma, unsigned long address,
			unsigned long end, pgprot_t newprot,
			unsigned long cp_flags)
{
	return 0;
}

static inline void __unmap_hugepage_range(struct mmu_gather *tlb,
			struct vm_area_struct *vma, unsigned long start,
			unsigned long end, struct folio *folio,
			zap_flags_t zap_flags)
{
	BUG();
}

static inline vm_fault_t hugetlb_fault(struct mm_struct *mm,
			struct vm_area_struct *vma, unsigned long address,
			unsigned int flags)
{
	BUG();
	return 0;
}

static inline void hugetlb_unshare_all_pmds(struct vm_area_struct *vma) { }

static inline void fixup_hugetlb_reservations(struct vm_area_struct *vma)
{
}

static inline void hugetlb_split(struct vm_area_struct *vma, unsigned long addr) {}

static inline int hugetlb_vma_lock_alloc(struct vm_area_struct *vma)
{
	return 0;
}


#ifndef pgd_write
static inline int pgd_write(pgd_t pgd)
{
	BUG();
	return 0;
}
#endif

#define HUGETLB_ANON_FILE "anon_hugepage"

enum {
	/*
	 * The file will be used as an shm file so shmfs accounting rules
	 * apply
	 */
	HUGETLB_SHMFS_INODE     = 1,
	/*
	 * The file is being created on the internal vfs mount and shmfs
	 * accounting rules do not apply
	 */
	HUGETLB_ANONHUGE_INODE  = 2,
};


#define is_file_hugepages(file)			false
static inline struct file *
hugetlb_file_setup(const char *name, size_t size, vma_flags_t acctflag,
		int creat_flags, int page_size_log)
{
	return ERR_PTR(-ENOSYS);
}

static inline struct hstate *hstate_inode(struct inode *i)
{
	return NULL;
}

unsigned long
hugetlb_get_unmapped_area(struct file *file, unsigned long addr,
				    unsigned long len, unsigned long pgoff,
				    unsigned long flags);

/*
 * huegtlb page specific state flags.  These flags are located in page.private
 * of the hugetlb head page.  Functions created via the below macros should be
 * used to manipulate these flags.
 *
 * HPG_restore_reserve - Set when a hugetlb page consumes a reservation at
 *	allocation time.  Cleared when page is fully instantiated.  Free
 *	routine checks flag to restore a reservation on error paths.
 *	Synchronization:  Examined or modified by code that knows it has
 *	the only reference to page.  i.e. After allocation but before use
 *	or when the page is being freed.
 * HPG_migratable  - Set after a newly allocated page is added to the page
 *	cache and/or page tables.  Indicates the page is a candidate for
 *	migration.
 *	Synchronization:  Initially set after new page allocation with no
 *	locking.  When examined and modified during migration processing
 *	(isolate, migrate, putback) the hugetlb_lock is held.
 * HPG_temporary - Set on a page that is temporarily allocated from the buddy
 *	allocator.  Typically used for migration target pages when no pages
 *	are available in the pool.  The hugetlb free page path will
 *	immediately free pages with this flag set to the buddy allocator.
 *	Synchronization: Can be set after huge page allocation from buddy when
 *	code knows it has only reference.  All other examinations and
 *	modifications require hugetlb_lock.
 * HPG_freed - Set when page is on the free lists.
 *	Synchronization: hugetlb_lock held for examination and modification.
 * HPG_vmemmap_optimized - Set when the vmemmap pages of the page are freed.
 * HPG_raw_hwp_unreliable - Set when the hugetlb page has a hwpoison sub-page
 *     that is not tracked by raw_hwp_page list.
 */
enum hugetlb_page_flags {
	HPG_restore_reserve = 0,
	HPG_migratable,
	HPG_temporary,
	HPG_freed,
	HPG_vmemmap_optimized,
	HPG_raw_hwp_unreliable,
	HPG_cma,
	__NR_HPAGEFLAGS,
};

/*
 * Macros to create test, set and clear function definitions for
 * hugetlb specific page flags.
 */
#define TESTHPAGEFLAG(uname, flname)				\
static inline bool						\
folio_test_hugetlb_##flname(struct folio *folio)		\
	{ return 0; }

#define SETHPAGEFLAG(uname, flname)				\
static inline void						\
folio_set_hugetlb_##flname(struct folio *folio) 		\
	{ }

#define CLEARHPAGEFLAG(uname, flname)				\
static inline void						\
folio_clear_hugetlb_##flname(struct folio *folio)		\
	{ }

#define HPAGEFLAG(uname, flname)				\
	TESTHPAGEFLAG(uname, flname)				\
	SETHPAGEFLAG(uname, flname)				\
	CLEARHPAGEFLAG(uname, flname)				\

/*
 * Create functions associated with hugetlb page flags
 */
HPAGEFLAG(RestoreReserve, restore_reserve)
HPAGEFLAG(Migratable, migratable)
HPAGEFLAG(Temporary, temporary)
HPAGEFLAG(Freed, freed)
HPAGEFLAG(VmemmapOptimized, vmemmap_optimized)
HPAGEFLAG(RawHwpUnreliable, raw_hwp_unreliable)
HPAGEFLAG(Cma, cma)

struct hstate {};

static inline unsigned long huge_page_mask_align(struct file *file)
{
	return 0;
}

static inline struct hugepage_subpool *hugetlb_folio_subpool(struct folio *folio)
{
	return NULL;
}

static inline struct folio *filemap_lock_hugetlb_folio(struct hstate *h,
				struct address_space *mapping, pgoff_t idx)
{
	return NULL;
}

static inline int isolate_or_dissolve_huge_folio(struct folio *folio,
						struct list_head *list)
{
	return -ENOMEM;
}

static inline int replace_free_hugepage_folios(unsigned long start_pfn,
		unsigned long end_pfn)
{
	return 0;
}

static inline void wait_for_freed_hugetlb_folios(void)
{
}

static inline struct folio *alloc_hugetlb_folio(struct vm_area_struct *vma,
					   unsigned long addr,
					   bool cow_from_owner)
{
	return NULL;
}

static inline struct folio *
alloc_hugetlb_folio_reserve(struct hstate *h, int preferred_nid,
			    nodemask_t *nmask, gfp_t gfp_mask)
{
	return NULL;
}

static inline struct folio *
alloc_hugetlb_folio_nodemask(struct hstate *h, int preferred_nid,
			nodemask_t *nmask, gfp_t gfp_mask,
			bool allow_alloc_fallback)
{
	return NULL;
}

static inline int __alloc_bootmem_huge_page(struct hstate *h)
{
	return 0;
}

static inline struct hstate *hstate_file(struct file *f)
{
	return NULL;
}

static inline struct hstate *hstate_sizelog(int page_size_log)
{
	return NULL;
}

static inline struct hstate *hstate_vma(struct vm_area_struct *vma)
{
	return NULL;
}

static inline struct hstate *folio_hstate(struct folio *folio)
{
	return NULL;
}

static inline struct hstate *size_to_hstate(unsigned long size)
{
	return NULL;
}

static inline unsigned long huge_page_size(struct hstate *h)
{
	return PAGE_SIZE;
}

static inline unsigned long huge_page_mask(struct hstate *h)
{
	return PAGE_MASK;
}

static inline unsigned int huge_page_order(struct hstate *h)
{
	return 0;
}

static inline unsigned int huge_page_shift(struct hstate *h)
{
	return PAGE_SHIFT;
}

static inline bool hstate_is_gigantic(struct hstate *h)
{
	return false;
}

static inline unsigned int pages_per_huge_page(struct hstate *h)
{
	return 1;
}

static inline unsigned hstate_index_to_shift(unsigned index)
{
	return 0;
}

static inline int hstate_index(struct hstate *h)
{
	return 0;
}

static inline int dissolve_free_hugetlb_folio(struct folio *folio)
{
	return 0;
}

static inline int dissolve_free_hugetlb_folios(unsigned long start_pfn,
					   unsigned long end_pfn)
{
	return 0;
}

static inline bool hugepage_migration_supported(struct hstate *h)
{
	return false;
}

static inline bool hugepage_movable_supported(struct hstate *h)
{
	return false;
}

static inline gfp_t htlb_alloc_mask(struct hstate *h)
{
	return 0;
}

static inline gfp_t htlb_modify_alloc_mask(struct hstate *h, gfp_t gfp_mask)
{
	return 0;
}

static inline bool htlb_allow_alloc_fallback(int reason)
{
	return false;
}

static inline spinlock_t *huge_pte_lockptr(struct hstate *h,
					   struct mm_struct *mm, pte_t *pte)
{
	return &mm->page_table_lock;
}

static inline void hugetlb_count_init(struct mm_struct *mm)
{
}

static inline void hugetlb_report_usage(struct seq_file *f, struct mm_struct *m)
{
}

static inline void hugetlb_count_sub(long l, struct mm_struct *mm)
{
}

static inline pte_t huge_ptep_clear_flush(struct vm_area_struct *vma,
					  unsigned long addr, pte_t *ptep)
{
#ifdef CONFIG_MMU
	return ptep_get(ptep);
#else
	return *ptep;
#endif
}

static inline void set_huge_pte_at(struct mm_struct *mm, unsigned long addr,
				   pte_t *ptep, pte_t pte, unsigned long sz)
{
}

static inline void hugetlb_register_node(struct node *node)
{
}

static inline void hugetlb_unregister_node(struct node *node)
{
}

static inline bool hugetlbfs_pagecache_present(
    struct hstate *h, struct vm_area_struct *vma, unsigned long address)
{
	return false;
}

static inline void hugetlb_bootmem_alloc(void)
{
}

static inline spinlock_t *huge_pte_lock(struct hstate *h,
					struct mm_struct *mm, pte_t *pte)
{
	spinlock_t *ptl;

	ptl = huge_pte_lockptr(h, mm, pte);
	spin_lock(ptl);
	return ptl;
}

static inline __init void hugetlb_cma_reserve(void)
{
}

static inline bool hugetlb_pmd_shared(pte_t *pte)
{
	return false;
}

bool want_pmd_share(struct vm_area_struct *vma, unsigned long addr);

#ifndef __HAVE_ARCH_FLUSH_HUGETLB_TLB_RANGE
/*
 * ARCHes with special requirements for evicting HUGETLB backing TLB entries can
 * implement this.
 */
#define flush_hugetlb_tlb_range(vma, addr, end)	flush_tlb_range(vma, addr, end)
#endif

static inline bool __vma_shareable_lock(struct vm_area_struct *vma)
{
	return (vma->vm_flags & VM_MAYSHARE) && vma->vm_private_data;
}

bool __vma_private_lock(struct vm_area_struct *vma);

/*
 * Safe version of huge_pte_offset() to check the locks.  See comments
 * above huge_pte_offset().
 */
static inline pte_t *
hugetlb_walk(struct vm_area_struct *vma, unsigned long addr, unsigned long sz)
{
	return huge_pte_offset(vma->vm_mm, addr, sz);
}

#endif /* _LINUX_HUGETLB_H */
