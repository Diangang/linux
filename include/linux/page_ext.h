/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __LINUX_PAGE_EXT_H
#define __LINUX_PAGE_EXT_H

#include <linux/types.h>
#include <linux/mmzone.h>
#include <linux/stacktrace.h>

struct pglist_data;

struct page_ext;

static inline bool early_page_ext_enabled(void)
{
	return false;
}

static inline void pgdat_page_ext_init(struct pglist_data *pgdat)
{
}

static inline void page_ext_init(void)
{
}

static inline void page_ext_init_flatmem_late(void)
{
}

static inline void page_ext_init_flatmem(void)
{
}

static inline struct page_ext *page_ext_get(const struct page *page)
{
	return NULL;
}

static inline struct page_ext *page_ext_from_phys(phys_addr_t phys)
{
	return NULL;
}

static inline void page_ext_put(struct page_ext *page_ext)
{
}
#endif /* __LINUX_PAGE_EXT_H */
