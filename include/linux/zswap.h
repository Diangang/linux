/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_ZSWAP_H
#define _LINUX_ZSWAP_H

#include <linux/types.h>
#include <linux/mm_types.h>

struct lruvec;

extern atomic_long_t zswap_stored_pages;


struct zswap_lruvec_state {};

static inline bool zswap_store(struct folio *folio)
{
	return false;
}

static inline int zswap_load(struct folio *folio)
{
	return -ENOENT;
}

static inline void zswap_invalidate(swp_entry_t swp) {}
static inline int zswap_swapon(int type, unsigned long nr_pages)
{
	return 0;
}
static inline void zswap_swapoff(int type) {}
static inline void zswap_memcg_offline_cleanup(struct mem_cgroup *memcg) {}
static inline void zswap_lruvec_state_init(struct lruvec *lruvec) {}
static inline void zswap_folio_swapin(struct folio *folio) {}

static inline bool zswap_is_enabled(void)
{
	return false;
}

static inline bool zswap_never_enabled(void)
{
	return true;
}


#endif /* _LINUX_ZSWAP_H */
