/* SPDX-License-Identifier: GPL-2.0 */
/*
 * page allocation tagging
 */
#ifndef _LINUX_PGALLOC_TAG_H
#define _LINUX_PGALLOC_TAG_H

#include <linux/alloc_tag.h>


static inline void clear_page_tag_ref(struct page *page) {}
static inline void alloc_tag_sec_init(void) {}
static inline void pgalloc_tag_split(struct folio *folio, int old_order, int new_order) {}
static inline void pgalloc_tag_swap(struct folio *new, struct folio *old) {}
static inline struct alloc_tag *pgalloc_tag_get(struct page *page) { return NULL; }


#endif /* _LINUX_PGALLOC_TAG_H */
