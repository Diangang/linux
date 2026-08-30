/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __LINUX_MEMORY_HOTPLUG_H
#define __LINUX_MEMORY_HOTPLUG_H

#include <linux/mmzone.h>
#include <linux/spinlock.h>
#include <linux/notifier.h>
#include <linux/bug.h>

struct page;
struct zone;
struct pglist_data;
struct mem_section;
struct memory_group;
struct resource;
struct vmem_altmap;
struct dev_pagemap;

/* Types for control the zone type of onlined and offlined memory */
enum mmop {
	/* Offline the memory. */
	MMOP_OFFLINE = 0,
	/* Online the memory. Zone depends, see default_zone_for_pfn(). */
	MMOP_ONLINE,
	/* Online the memory to ZONE_NORMAL. */
	MMOP_ONLINE_KERNEL,
	/* Online the memory to ZONE_MOVABLE. */
	MMOP_ONLINE_MOVABLE,
};

#define pfn_to_online_page(pfn)			\
({						\
	struct page *___page = NULL;		\
	if (pfn_valid(pfn))			\
		___page = pfn_to_page(pfn);	\
	___page;				\
 })

static inline unsigned zone_span_seqbegin(struct zone *zone)
{
	return 0;
}
static inline int zone_span_seqretry(struct zone *zone, unsigned iv)
{
	return 0;
}
static inline void zone_span_writelock(struct zone *zone) {}
static inline void zone_span_writeunlock(struct zone *zone) {}
static inline void zone_seqlock_init(struct zone *zone) {}

static inline int try_online_node(int nid)
{
	return 0;
}

static inline void get_online_mems(void) {}
static inline void put_online_mems(void) {}

static inline void mem_hotplug_begin(void) {}
static inline void mem_hotplug_done(void) {}

static inline bool movable_node_is_enabled(void)
{
	return false;
}

static inline bool mhp_supports_memmap_on_memory(void)
{
	return false;
}

static inline void pgdat_kswapd_lock(pg_data_t *pgdat) {}
static inline void pgdat_kswapd_unlock(pg_data_t *pgdat) {}
static inline void pgdat_kswapd_lock_init(pg_data_t *pgdat) {}

/*
 * Keep this declaration outside CONFIG_MEMORY_HOTPLUG as some
 * platforms might override and use arch_get_mappable_range()
 * for internal non memory hotplug purposes.
 */
struct range arch_get_mappable_range(void);

/*
 * Stub functions for when hotplug is off
 */
static inline void pgdat_resize_lock(struct pglist_data *p, unsigned long *f) {}
static inline void pgdat_resize_unlock(struct pglist_data *p, unsigned long *f) {}
static inline void pgdat_resize_init(struct pglist_data *pgdat) {}

static inline void try_offline_node(int nid) {}

static inline int offline_pages(unsigned long start_pfn, unsigned long nr_pages,
				struct zone *zone, struct memory_group *group)
{
	return -EINVAL;
}

static inline int remove_memory(u64 start, u64 size)
{
	return -EBUSY;
}

static inline void __remove_memory(u64 start, u64 size) {}


#endif /* __LINUX_MEMORY_HOTPLUG_H */
