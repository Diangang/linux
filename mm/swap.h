/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _MM_SWAP_H
#define _MM_SWAP_H

#include <linux/atomic.h> /* for atomic_long_t */
struct mempolicy;
struct swap_iocb;

extern int page_cluster;

#define SWAPFILE_CLUSTER	256
#define swap_entry_order(order)	0

extern struct swap_info_struct *swap_info[];

/*
 * We use this to track usage of a cluster. A cluster is a block of swap disk
 * space with SWAPFILE_CLUSTER pages long and naturally aligns in disk. All
 * free clusters are organized into a list. We fetch an entry from the list to
 * get a free cluster.
 *
 * The flags field determines if a cluster is free. This is
 * protected by cluster lock.
 */
struct swap_cluster_info {
	spinlock_t lock;	/*
				 * Protect swap_cluster_info fields
				 * other than list, and swap_info_struct->swap_map
				 * elements corresponding to the swap cluster.
				 */
	u16 count;
	u8 flags;
	u8 order;
	atomic_long_t __rcu *table;	/* Swap table entries, see mm/swap_table.h */
	unsigned int *extend_table;	/* For large swap count, protected by ci->lock */
	struct list_head list;
};

/* All on-list cluster must have a non-zero flag. */
enum swap_cluster_flags {
	CLUSTER_FLAG_NONE = 0, /* For temporary off-list cluster */
	CLUSTER_FLAG_FREE,
	CLUSTER_FLAG_NONFULL,
	CLUSTER_FLAG_FRAG,
	/* Clusters with flags above are allocatable */
	CLUSTER_FLAG_USABLE = CLUSTER_FLAG_FRAG,
	CLUSTER_FLAG_FULL,
	CLUSTER_FLAG_DISCARD,
	CLUSTER_FLAG_MAX,
};

struct swap_iocb;
static inline struct swap_cluster_info *swap_cluster_lock(
	struct swap_info_struct *si, pgoff_t offset, bool irq)
{
	return NULL;
}

static inline struct swap_cluster_info *swap_cluster_get_and_lock(
		struct folio *folio)
{
	return NULL;
}

static inline struct swap_cluster_info *swap_cluster_get_and_lock_irq(
		struct folio *folio)
{
	return NULL;
}

static inline void swap_cluster_unlock(struct swap_cluster_info *ci)
{
}

static inline void swap_cluster_unlock_irq(struct swap_cluster_info *ci)
{
}

static inline struct swap_info_struct *__swap_entry_to_info(swp_entry_t entry)
{
	return NULL;
}

static inline int folio_alloc_swap(struct folio *folio)
{
	return -EINVAL;
}

static inline int folio_dup_swap(struct folio *folio, struct page *page)
{
	return -EINVAL;
}

static inline void folio_put_swap(struct folio *folio, struct page *page)
{
}

static inline void swap_read_folio(struct folio *folio, struct swap_iocb **plug)
{
}

static inline void swap_write_unplug(struct swap_iocb *sio)
{
}

static inline struct address_space *swap_address_space(swp_entry_t entry)
{
	return NULL;
}

static inline bool folio_matches_swap_entry(const struct folio *folio, swp_entry_t entry)
{
	return false;
}

static inline void show_swap_cache_info(void)
{
}

static inline struct folio *swap_cluster_readahead(swp_entry_t entry,
			gfp_t gfp_mask, struct mempolicy *mpol, pgoff_t ilx)
{
	return NULL;
}

static inline struct folio *swapin_readahead(swp_entry_t swp, gfp_t gfp_mask,
			struct vm_fault *vmf)
{
	return NULL;
}

static inline struct folio *swapin_folio(swp_entry_t entry, struct folio *folio)
{
	return NULL;
}

static inline void swap_update_readahead(struct folio *folio,
		struct vm_area_struct *vma, unsigned long addr)
{
}

static inline int swap_writeout(struct folio *folio,
		struct swap_iocb **swap_plug)
{
	return 0;
}

static inline int swap_retry_table_alloc(swp_entry_t entry, gfp_t gfp)
{
	return -EINVAL;
}

static inline bool swap_cache_has_folio(swp_entry_t entry)
{
	return false;
}

static inline struct folio *swap_cache_get_folio(swp_entry_t entry)
{
	return NULL;
}

static inline void *swap_cache_get_shadow(swp_entry_t entry)
{
	return NULL;
}

static inline void swap_cache_del_folio(struct folio *folio)
{
}

static inline void __swap_cache_del_folio(struct swap_cluster_info *ci,
		struct folio *folio, swp_entry_t entry, void *shadow)
{
}

static inline void __swap_cache_replace_folio(struct swap_cluster_info *ci,
		struct folio *old, struct folio *new)
{
}

static inline unsigned int folio_swap_flags(struct folio *folio)
{
	return 0;
}

static inline int swap_zeromap_batch(swp_entry_t entry, int max_nr,
		bool *has_zeromap)
{
	return 0;
}

static inline int non_swapcache_batch(swp_entry_t entry, int max_nr)
{
	return 0;
}
#endif /* _MM_SWAP_H */
