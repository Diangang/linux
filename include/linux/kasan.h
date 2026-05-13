/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_KASAN_H
#define _LINUX_KASAN_H

#include <linux/kasan-enabled.h>
#include <linux/kasan-tags.h>
#include <linux/types.h>

struct kmem_cache;
struct page;
struct slab;
struct task_struct;
struct vm_struct;

typedef unsigned int __bitwise kasan_vmalloc_flags_t;

#define KASAN_VMALLOC_NONE		((__force kasan_vmalloc_flags_t)0x00u)
#define KASAN_VMALLOC_INIT		((__force kasan_vmalloc_flags_t)0x01u)
#define KASAN_VMALLOC_VM_ALLOC		((__force kasan_vmalloc_flags_t)0x02u)
#define KASAN_VMALLOC_PROT_NORMAL	((__force kasan_vmalloc_flags_t)0x04u)
#define KASAN_VMALLOC_KEEP_TAG		((__force kasan_vmalloc_flags_t)0x08u)

#define KASAN_VMALLOC_PAGE_RANGE	0x1
#define KASAN_VMALLOC_TLB_FLUSH		0x2

static inline int kasan_add_zero_shadow(void *start, unsigned long size) { return 0; }
static inline void kasan_remove_zero_shadow(void *start, unsigned long size) { }
static inline void kasan_enable_current(void) { }
static inline void kasan_disable_current(void) { }
static inline bool kasan_has_integrated_init(void) { return false; }

static inline void kasan_unpoison_range(const void *address, size_t size) { }
static inline void kasan_poison_pages(struct page *page, unsigned int order, bool init) { }
static inline bool kasan_unpoison_pages(struct page *page, unsigned int order, bool init)
{
	return false;
}
static inline void kasan_poison_slab(struct slab *slab) { }
static inline void kasan_unpoison_new_object(struct kmem_cache *cache, void *object) { }
static inline void kasan_poison_new_object(struct kmem_cache *cache, void *object) { }
static inline void *kasan_init_slab_obj(struct kmem_cache *cache, const void *object)
{
	return (void *)object;
}
static inline bool kasan_slab_pre_free(struct kmem_cache *s, void *object)
{
	return false;
}
static inline bool kasan_slab_free(struct kmem_cache *s, void *object,
				   bool init, bool still_accessible,
				   bool no_quarantine)
{
	return false;
}
static inline void kasan_kfree_large(void *ptr) { }
static inline void *kasan_slab_alloc(struct kmem_cache *s, void *object,
				     gfp_t flags, bool init)
{
	return object;
}
static inline void *kasan_kmalloc(struct kmem_cache *s, const void *object,
				  size_t size, gfp_t flags)
{
	return (void *)object;
}
static inline void *kasan_kmalloc_large(const void *ptr, size_t size, gfp_t flags)
{
	return (void *)ptr;
}
static inline void *kasan_krealloc(const void *object, size_t new_size, gfp_t flags)
{
	return (void *)object;
}
static inline bool kasan_mempool_poison_pages(struct page *page, unsigned int order)
{
	return true;
}
static inline void kasan_mempool_unpoison_pages(struct page *page, unsigned int order) { }
static inline bool kasan_mempool_poison_object(void *ptr) { return true; }
static inline void kasan_mempool_unpoison_object(void *ptr, size_t size) { }
static inline bool kasan_check_byte(const void *address) { return true; }
static inline void kasan_unpoison_task_stack(struct task_struct *task) { }
static inline void kasan_unpoison_task_stack_below(const void *watermark) { }

static inline size_t kasan_metadata_size(struct kmem_cache *cache, bool in_object)
{
	return 0;
}
static inline void kasan_cache_create(struct kmem_cache *cache,
				      unsigned int *size,
				      slab_flags_t *flags) { }
static inline void kasan_cache_shrink(struct kmem_cache *cache) { }
static inline void kasan_cache_shutdown(struct kmem_cache *cache) { }
static inline void kasan_record_aux_stack(void *ptr) { }
static inline void *kasan_reset_tag(const void *addr) { return (void *)addr; }

static inline void kasan_init_generic(void) { }
static inline void kasan_init_sw_tags(void) { }
static inline void kasan_init_hw_tags_cpu(void) { }
static inline void kasan_init_hw_tags(void) { }

static inline void kasan_populate_early_vm_area_shadow(void *start,
						       unsigned long size) { }
static inline int kasan_populate_vmalloc(unsigned long start,
					 unsigned long size, gfp_t gfp_mask)
{
	return 0;
}
static inline void kasan_release_vmalloc(unsigned long start, unsigned long end,
					 unsigned long free_region_start,
					 unsigned long free_region_end,
					 unsigned long flags) { }
static inline void *kasan_unpoison_vmalloc(const void *start, unsigned long size,
					   kasan_vmalloc_flags_t flags)
{
	return (void *)start;
}
static inline void kasan_poison_vmalloc(const void *start, unsigned long size) { }
static inline void kasan_unpoison_vmap_areas(struct vm_struct **vms, int nr_vms,
					     kasan_vmalloc_flags_t flags) { }
static inline void kasan_vrealloc(const void *start, unsigned long old_size,
				  unsigned long new_size) { }
static inline int kasan_alloc_module_shadow(void *addr, size_t size, gfp_t gfp_mask)
{
	return 0;
}
static inline void kasan_free_module_shadow(const struct vm_struct *vm) { }
static inline void kasan_non_canonical_hook(unsigned long addr) { }

#endif /* _LINUX_KASAN_H */
