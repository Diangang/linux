// SPDX-License-Identifier: GPL-2.0
/*
 * HugeTLB Vmemmap Optimization (HVO)
 *
 * Copyright (c) 2020, ByteDance. All rights reserved.
 *
 *     Author: Muchun Song <songmuchun@bytedance.com>
 */
#ifndef _LINUX_HUGETLB_VMEMMAP_H
#define _LINUX_HUGETLB_VMEMMAP_H
#include <linux/hugetlb.h>
#include <linux/io.h>
#include <linux/memblock.h>

/*
 * Reserve one vmemmap page, all vmemmap addresses are mapped to it. See
 * Documentation/mm/vmemmap_dedup.rst.
 */
#define HUGETLB_VMEMMAP_RESERVE_SIZE	PAGE_SIZE
#define HUGETLB_VMEMMAP_RESERVE_PAGES	(HUGETLB_VMEMMAP_RESERVE_SIZE / sizeof(struct page))

static inline int hugetlb_vmemmap_restore_folio(const struct hstate *h, struct folio *folio)
{
	return 0;
}

static inline long hugetlb_vmemmap_restore_folios(const struct hstate *h,
					struct list_head *folio_list,
					struct list_head *non_hvo_folios)
{
	list_splice_init(folio_list, non_hvo_folios);
	return 0;
}

static inline void hugetlb_vmemmap_optimize_folio(const struct hstate *h, struct folio *folio)
{
}

static inline void hugetlb_vmemmap_optimize_folios(struct hstate *h, struct list_head *folio_list)
{
}

static inline void hugetlb_vmemmap_optimize_bootmem_folios(struct hstate *h,
						struct list_head *folio_list)
{
}

static inline void hugetlb_vmemmap_init_early(int nid)
{
}

static inline void hugetlb_vmemmap_init_late(int nid)
{
}

static inline unsigned int hugetlb_vmemmap_optimizable_size(const struct hstate *h)
{
	return 0;
}

static inline bool hugetlb_vmemmap_optimizable(const struct hstate *h)
{
	return hugetlb_vmemmap_optimizable_size(h) != 0;
}
#endif /* _LINUX_HUGETLB_VMEMMAP_H */
