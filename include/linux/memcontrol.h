/* SPDX-License-Identifier: GPL-2.0-or-later */
/* memcontrol.h - Memory Controller
 *
 * Copyright IBM Corporation, 2007
 * Author Balbir Singh <balbir@linux.vnet.ibm.com>
 *
 * Copyright 2007 OpenVZ SWsoft Inc
 * Author: Pavel Emelianov <xemul@openvz.org>
 */

#ifndef _LINUX_MEMCONTROL_H
#define _LINUX_MEMCONTROL_H
#include <linux/cgroup.h>
#include <linux/vm_event_item.h>
#include <linux/hardirq.h>
#include <linux/jump_label.h>
#include <linux/kernel.h>
#include <linux/page_counter.h>
#include <linux/vmpressure.h>
#include <linux/eventfd.h>
#include <linux/mm.h>
#include <linux/vmstat.h>
#include <linux/writeback.h>
#include <linux/page-flags.h>
#include <linux/shrinker.h>

struct mem_cgroup;
struct obj_cgroup;
struct page;
struct mm_struct;
struct kmem_cache;

/* Cgroup-specific page state, on top of universal node page state */
enum memcg_stat_item {
	MEMCG_SWAP = NR_VM_NODE_STAT_ITEMS,
	MEMCG_PERCPU_B,
	MEMCG_KMEM,
	MEMCG_ZSWAP_B,
	MEMCG_ZSWAPPED,
	MEMCG_ZSWAP_INCOMP,
	MEMCG_NR_STAT,
};

enum memcg_memory_event {
	MEMCG_LOW,
	MEMCG_HIGH,
	MEMCG_MAX,
	MEMCG_OOM,
	MEMCG_OOM_KILL,
	MEMCG_OOM_GROUP_KILL,
	MEMCG_SWAP_HIGH,
	MEMCG_SWAP_MAX,
	MEMCG_SWAP_FAIL,
	MEMCG_NR_MEMORY_EVENTS,
};

struct mem_cgroup_reclaim_cookie {
	pg_data_t *pgdat;
	int generation;
};


#define __OBJEXTS_ALLOC_FAIL	(1UL << 0)
#define __FIRST_OBJEXT_FLAG	(1UL << 0)


enum objext_flags {
	/*
	 * Use bit 0 with zero other bits to signal that slabobj_ext vector
	 * failed to allocate. The same bit 0 with valid upper bits means
	 * MEMCG_DATA_OBJEXTS.
	 */
	OBJEXTS_ALLOC_FAIL = __OBJEXTS_ALLOC_FAIL,
	__OBJEXTS_FLAG_UNUSED = __FIRST_OBJEXT_FLAG,
	/* the next bit after the last actual flag */
	__NR_OBJEXTS_FLAGS  = (__FIRST_OBJEXT_FLAG << 1),
};

#define OBJEXTS_FLAGS_MASK (__NR_OBJEXTS_FLAGS - 1)


#define MEM_CGROUP_ID_SHIFT	0

#define root_mem_cgroup		(NULL)

static inline struct mem_cgroup *folio_memcg(struct folio *folio)
{
	return NULL;
}

static inline bool folio_memcg_charged(struct folio *folio)
{
	return false;
}

static inline struct mem_cgroup *folio_memcg_check(struct folio *folio)
{
	return NULL;
}

static inline struct mem_cgroup *page_memcg_check(struct page *page)
{
	return NULL;
}

static inline struct mem_cgroup *get_mem_cgroup_from_objcg(struct obj_cgroup *objcg)
{
	return NULL;
}

static inline bool folio_memcg_kmem(struct folio *folio)
{
	return false;
}

static inline bool PageMemcgKmem(struct page *page)
{
	return false;
}

static inline bool mem_cgroup_is_root(struct mem_cgroup *memcg)
{
	return true;
}

static inline bool obj_cgroup_is_root(const struct obj_cgroup *objcg)
{
	return true;
}

static inline bool mem_cgroup_disabled(void)
{
	return true;
}

static inline void memcg_memory_event(struct mem_cgroup *memcg,
				      enum memcg_memory_event event)
{
}

static inline void memcg_memory_event_mm(struct mm_struct *mm,
					 enum memcg_memory_event event)
{
}

static inline void mem_cgroup_protection(struct mem_cgroup *root,
					 struct mem_cgroup *memcg,
					 unsigned long *min,
					 unsigned long *low,
					 unsigned long *usage)
{
	*min = *low = *usage = 0;
}

static inline void mem_cgroup_calculate_protection(struct mem_cgroup *root,
						   struct mem_cgroup *memcg)
{
}

static inline bool mem_cgroup_unprotected(struct mem_cgroup *target,
					  struct mem_cgroup *memcg)
{
	return true;
}
static inline bool mem_cgroup_below_low(struct mem_cgroup *target,
					struct mem_cgroup *memcg)
{
	return false;
}

static inline bool mem_cgroup_below_min(struct mem_cgroup *target,
					struct mem_cgroup *memcg)
{
	return false;
}

static inline int mem_cgroup_charge(struct folio *folio,
		struct mm_struct *mm, gfp_t gfp)
{
	return 0;
}

static inline int mem_cgroup_charge_hugetlb(struct folio* folio, gfp_t gfp)
{
        return 0;
}

static inline int mem_cgroup_swapin_charge_folio(struct folio *folio,
			struct mm_struct *mm, gfp_t gfp, swp_entry_t entry)
{
	return 0;
}

static inline void mem_cgroup_uncharge(struct folio *folio)
{
}

static inline void mem_cgroup_uncharge_folios(struct folio_batch *folios)
{
}

static inline void mem_cgroup_replace_folio(struct folio *old,
		struct folio *new)
{
}

static inline void mem_cgroup_migrate(struct folio *old, struct folio *new)
{
}

static inline struct lruvec *mem_cgroup_lruvec(struct mem_cgroup *memcg,
					       struct pglist_data *pgdat)
{
	return &pgdat->__lruvec;
}

static inline struct lruvec *folio_lruvec(struct folio *folio)
{
	struct pglist_data *pgdat = folio_pgdat(folio);
	return &pgdat->__lruvec;
}

static inline struct mem_cgroup *parent_mem_cgroup(struct mem_cgroup *memcg)
{
	return NULL;
}

static inline bool mm_match_cgroup(struct mm_struct *mm,
		struct mem_cgroup *memcg)
{
	return true;
}

static inline struct mem_cgroup *get_mem_cgroup_from_mm(struct mm_struct *mm)
{
	return NULL;
}

static inline struct mem_cgroup *get_mem_cgroup_from_current(void)
{
	return NULL;
}

static inline struct mem_cgroup *get_mem_cgroup_from_folio(struct folio *folio)
{
	return NULL;
}

static inline
struct mem_cgroup *mem_cgroup_from_css(struct cgroup_subsys_state *css)
{
	return NULL;
}

static inline void obj_cgroup_get(struct obj_cgroup *objcg)
{
}

static inline void obj_cgroup_put(struct obj_cgroup *objcg)
{
}

static inline bool mem_cgroup_tryget(struct mem_cgroup *memcg)
{
	return true;
}

static inline bool mem_cgroup_tryget_online(struct mem_cgroup *memcg)
{
	return true;
}

static inline void mem_cgroup_put(struct mem_cgroup *memcg)
{
}

static inline struct lruvec *folio_lruvec_lock(struct folio *folio)
{
	struct pglist_data *pgdat = folio_pgdat(folio);

	rcu_read_lock();
	spin_lock(&pgdat->__lruvec.lru_lock);
	return &pgdat->__lruvec;
}

static inline struct lruvec *folio_lruvec_lock_irq(struct folio *folio)
{
	struct pglist_data *pgdat = folio_pgdat(folio);

	rcu_read_lock();
	spin_lock_irq(&pgdat->__lruvec.lru_lock);
	return &pgdat->__lruvec;
}

static inline struct lruvec *folio_lruvec_lock_irqsave(struct folio *folio,
		unsigned long *flagsp)
{
	struct pglist_data *pgdat = folio_pgdat(folio);

	rcu_read_lock();
	spin_lock_irqsave(&pgdat->__lruvec.lru_lock, *flagsp);
	return &pgdat->__lruvec;
}

static inline struct mem_cgroup *
mem_cgroup_iter(struct mem_cgroup *root,
		struct mem_cgroup *prev,
		struct mem_cgroup_reclaim_cookie *reclaim)
{
	return NULL;
}

static inline void mem_cgroup_iter_break(struct mem_cgroup *root,
					 struct mem_cgroup *prev)
{
}

static inline void mem_cgroup_scan_tasks(struct mem_cgroup *memcg,
		int (*fn)(struct task_struct *, void *), void *arg)
{
}

static inline unsigned short mem_cgroup_private_id(struct mem_cgroup *memcg)
{
	return 0;
}

static inline struct mem_cgroup *mem_cgroup_from_private_id(unsigned short id)
{
	WARN_ON_ONCE(id);
	/* XXX: This should always return root_mem_cgroup */
	return NULL;
}

static inline u64 mem_cgroup_id(struct mem_cgroup *memcg)
{
	return 0;
}

static inline struct mem_cgroup *mem_cgroup_get_from_id(u64 id)
{
	return NULL;
}

static inline struct mem_cgroup *mem_cgroup_from_seq(struct seq_file *m)
{
	return NULL;
}

static inline struct mem_cgroup *lruvec_memcg(struct lruvec *lruvec)
{
	return NULL;
}

static inline bool mem_cgroup_online(struct mem_cgroup *memcg)
{
	return true;
}

static inline
unsigned long mem_cgroup_get_zone_lru_size(struct lruvec *lruvec,
		enum lru_list lru, int zone_idx)
{
	return 0;
}

static inline unsigned long mem_cgroup_get_max(struct mem_cgroup *memcg)
{
	return 0;
}

static inline void
mem_cgroup_print_oom_context(struct mem_cgroup *memcg, struct task_struct *p)
{
}

static inline void
mem_cgroup_print_oom_meminfo(struct mem_cgroup *memcg)
{
}

static inline void mem_cgroup_handle_over_high(gfp_t gfp_mask)
{
}

static inline struct mem_cgroup *mem_cgroup_get_oom_group(
	struct task_struct *victim, struct mem_cgroup *oom_domain)
{
	return NULL;
}

static inline void mem_cgroup_print_oom_group(struct mem_cgroup *memcg)
{
}

static inline void mod_memcg_state(struct mem_cgroup *memcg,
				   enum memcg_stat_item idx,
				   int nr)
{
}

static inline void mod_memcg_page_state(struct page *page,
					enum memcg_stat_item idx, int val)
{
}

static inline unsigned long memcg_page_state(struct mem_cgroup *memcg, int idx)
{
	return 0;
}

static inline unsigned long memcg_page_state_output(struct mem_cgroup *memcg, int item)
{
	return 0;
}

static inline bool memcg_stat_item_valid(int idx)
{
	return false;
}

static inline bool memcg_vm_event_item_valid(enum vm_event_item idx)
{
	return false;
}

static inline unsigned long lruvec_page_state(struct lruvec *lruvec,
					      enum node_stat_item idx)
{
	return node_page_state(lruvec_pgdat(lruvec), idx);
}

static inline unsigned long lruvec_page_state_local(struct lruvec *lruvec,
						    enum node_stat_item idx)
{
	return node_page_state(lruvec_pgdat(lruvec), idx);
}

static inline void mem_cgroup_flush_stats(struct mem_cgroup *memcg)
{
}

static inline void mem_cgroup_flush_stats_ratelimited(struct mem_cgroup *memcg)
{
}

static inline void mod_lruvec_kmem_state(void *p, enum node_stat_item idx,
					 int val)
{
	struct page *page = virt_to_head_page(p);

	mod_node_page_state(page_pgdat(page), idx, val);
}

static inline void count_memcg_events(struct mem_cgroup *memcg,
					enum vm_event_item idx,
					unsigned long count)
{
}

static inline void count_memcg_folio_events(struct folio *folio,
		enum vm_event_item idx, unsigned long nr)
{
}

static inline void count_memcg_events_mm(struct mm_struct *mm,
					enum vm_event_item idx, unsigned long count)
{
}

static inline
void count_memcg_event_mm(struct mm_struct *mm, enum vm_event_item idx)
{
}

static inline void split_page_memcg(struct page *first, unsigned order)
{
}

static inline void folio_split_memcg_refs(struct folio *folio,
		unsigned old_order, unsigned new_order)
{
}

static inline u64 cgroup_id_from_mm(struct mm_struct *mm)
{
	return 0;
}

static inline void mem_cgroup_flush_workqueue(void) { }

static inline int mem_cgroup_init(void) { return 0; }

/*
 * Extended information for slab objects stored as an array in page->memcg_data
 * if MEMCG_DATA_OBJEXTS is set.
 */
struct slabobj_ext {
} __aligned(8);

static inline struct lruvec *parent_lruvec(struct lruvec *lruvec)
{
	struct mem_cgroup *memcg;

	memcg = lruvec_memcg(lruvec);
	if (!memcg)
		return NULL;
	memcg = parent_mem_cgroup(memcg);
	if (!memcg)
		return NULL;
	return mem_cgroup_lruvec(memcg, lruvec_pgdat(lruvec));
}

static inline void lruvec_lock_irq(struct lruvec *lruvec)
{
	rcu_read_lock();
	spin_lock_irq(&lruvec->lru_lock);
}

static inline void lruvec_unlock(struct lruvec *lruvec)
{
	spin_unlock(&lruvec->lru_lock);
	rcu_read_unlock();
}

static inline void lruvec_unlock_irq(struct lruvec *lruvec)
{
	spin_unlock_irq(&lruvec->lru_lock);
	rcu_read_unlock();
}

static inline void lruvec_unlock_irqrestore(struct lruvec *lruvec, unsigned long flags)
{
	spin_unlock_irqrestore(&lruvec->lru_lock, flags);
	rcu_read_unlock();
}

/* Test requires a stable folio->memcg binding, see folio_memcg() */
static inline bool folio_matches_lruvec(struct folio *folio,
		struct lruvec *lruvec)
{
	return lruvec_pgdat(lruvec) == folio_pgdat(folio) &&
	       lruvec_memcg(lruvec) == folio_memcg(folio);
}

/* Don't lock again iff page's lruvec locked */
static inline struct lruvec *folio_lruvec_relock_irq(struct folio *folio,
		struct lruvec *locked_lruvec)
{
	if (locked_lruvec) {
		if (folio_matches_lruvec(folio, locked_lruvec))
			return locked_lruvec;

		lruvec_unlock_irq(locked_lruvec);
	}

	return folio_lruvec_lock_irq(folio);
}

/* Don't lock again iff folio's lruvec locked */
static inline void folio_lruvec_relock_irqsave(struct folio *folio,
		struct lruvec **lruvecp, unsigned long *flags)
{
	if (*lruvecp) {
		if (folio_matches_lruvec(folio, *lruvecp))
			return;

		lruvec_unlock_irqrestore(*lruvecp, *flags);
	}

	*lruvecp = folio_lruvec_lock_irqsave(folio, flags);
}


static inline struct wb_domain *mem_cgroup_wb_domain(struct bdi_writeback *wb)
{
	return NULL;
}

static inline void mem_cgroup_wb_stats(struct bdi_writeback *wb,
				       unsigned long *pfilepages,
				       unsigned long *pheadroom,
				       unsigned long *pdirty,
				       unsigned long *pwriteback)
{
}

static inline void mem_cgroup_track_foreign_dirty(struct folio *folio,
						  struct bdi_writeback *wb)
{
}

static inline void mem_cgroup_flush_foreign(struct bdi_writeback *wb)
{
}


static inline void set_shrinker_bit(struct mem_cgroup *memcg,
				    int nid, int shrinker_id)
{
}

static inline int shrinker_id(struct shrinker *shrinker)
{
	return -1;
}

static inline bool mem_cgroup_kmem_disabled(void)
{
	return true;
}

static inline int memcg_kmem_charge_page(struct page *page, gfp_t gfp,
					 int order)
{
	return 0;
}

static inline void memcg_kmem_uncharge_page(struct page *page, int order)
{
}

static inline int __memcg_kmem_charge_page(struct page *page, gfp_t gfp,
					   int order)
{
	return 0;
}

static inline void __memcg_kmem_uncharge_page(struct page *page, int order)
{
}

static inline struct obj_cgroup *get_obj_cgroup_from_folio(struct folio *folio)
{
	return NULL;
}

static inline bool memcg_kmem_online(void)
{
	return false;
}

static inline int memcg_kmem_id(struct mem_cgroup *memcg)
{
	return -1;
}

static inline struct mem_cgroup *mem_cgroup_from_virt(void *p)
{
	return NULL;
}

static inline void count_objcg_events(struct obj_cgroup *objcg,
				      enum vm_event_item idx,
				      unsigned long count)
{
}

static inline ino_t page_cgroup_ino(struct page *page)
{
	return 0;
}

static inline void mem_cgroup_node_filter_allowed(struct mem_cgroup *memcg,
						  nodemask_t *mask)
{
}

static inline void mem_cgroup_show_protected_memory(struct mem_cgroup *memcg)
{
}

static inline bool memcg_is_dying(struct mem_cgroup *memcg)
{
	return false;
}

static inline bool obj_cgroup_may_zswap(struct obj_cgroup *objcg)
{
	return true;
}
static inline void obj_cgroup_charge_zswap(struct obj_cgroup *objcg,
					   size_t size)
{
}
static inline void obj_cgroup_uncharge_zswap(struct obj_cgroup *objcg,
					     size_t size)
{
}
static inline bool mem_cgroup_zswap_writeback_enabled(struct mem_cgroup *memcg)
{
	/* if zswap is disabled, do not block pages going to the swapping device */
	return true;
}


/* Cgroup v1-related declarations */

static inline
unsigned long memcg1_soft_limit_reclaim(pg_data_t *pgdat, int order,
					gfp_t gfp_mask,
					unsigned long *total_scanned)
{
	return 0;
}

static inline bool task_in_memcg_oom(struct task_struct *p)
{
	return false;
}

static inline bool mem_cgroup_oom_synchronize(bool wait)
{
	return false;
}

static inline void mem_cgroup_enter_user_fault(void)
{
}

static inline void mem_cgroup_exit_user_fault(void)
{
}

static inline void memcg1_swapout(struct folio *folio, swp_entry_t entry)
{
}

static inline void memcg1_swapin(swp_entry_t entry, unsigned int nr_pages)
{
}


#endif /* _LINUX_MEMCONTROL_H */
