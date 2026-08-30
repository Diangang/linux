/* SPDX-License-Identifier: GPL-2.0 */
/*
 * KMSAN API for subsystems.
 *
 * Copyright (C) 2017-2022 Google LLC
 * Author: Alexander Potapenko <glider@google.com>
 *
 */
#ifndef _LINUX_KMSAN_H
#define _LINUX_KMSAN_H

#include <linux/dma-direction.h>
#include <linux/gfp.h>
#include <linux/kmsan-checks.h>
#include <linux/types.h>

struct page;
struct kmem_cache;
struct task_struct;
struct scatterlist;
struct urb;


static inline void kmsan_init_shadow(void)
{
}

static inline void kmsan_init_runtime(void)
{
}

static inline bool __must_check kmsan_memblock_free_pages(struct page *page,
							  unsigned int order)
{
	return true;
}

static inline void kmsan_task_create(struct task_struct *task)
{
}

static inline void kmsan_task_exit(struct task_struct *task)
{
}

static inline void kmsan_alloc_page(struct page *page, unsigned int order,
				    gfp_t flags)
{
}

static inline void kmsan_free_page(struct page *page, unsigned int order)
{
}

static inline void kmsan_copy_page_meta(struct page *dst, struct page *src)
{
}

static inline void kmsan_slab_alloc(struct kmem_cache *s, void *object,
				    gfp_t flags)
{
}

static inline void kmsan_slab_free(struct kmem_cache *s, void *object)
{
}

static inline void kmsan_kmalloc_large(const void *ptr, size_t size,
				       gfp_t flags)
{
}

static inline void kmsan_kfree_large(const void *ptr)
{
}

static inline int __must_check kmsan_vmap_pages_range_noflush(
	unsigned long start, unsigned long end, pgprot_t prot,
	struct page **pages, unsigned int page_shift, gfp_t gfp_mask)
{
	return 0;
}

static inline void kmsan_vunmap_range_noflush(unsigned long start,
					      unsigned long end)
{
}

static inline int __must_check kmsan_ioremap_page_range(unsigned long start,
							unsigned long end,
							phys_addr_t phys_addr,
							pgprot_t prot,
							unsigned int page_shift)
{
	return 0;
}

static inline void kmsan_iounmap_page_range(unsigned long start,
					    unsigned long end)
{
}

static inline void kmsan_handle_dma(phys_addr_t phys, size_t size,
				    enum dma_data_direction dir)
{
}

static inline void kmsan_handle_dma_sg(struct scatterlist *sg, int nents,
				       enum dma_data_direction dir)
{
}

static inline void kmsan_handle_urb(const struct urb *urb, bool is_out)
{
}

static inline void kmsan_unpoison_entry_regs(const struct pt_regs *regs)
{
}

static inline void kmsan_enable_current(void)
{
}

static inline void kmsan_disable_current(void)
{
}

static inline void *memset_no_sanitize_memory(void *s, int c, size_t n)
{
	return memset(s, c, n);
}

#define KMSAN_WARN_ON WARN_ON


#endif /* _LINUX_KMSAN_H */
