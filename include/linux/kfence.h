/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_KFENCE_H
#define _LINUX_KFENCE_H

#include <linux/types.h>

struct kmem_cache;
struct kmem_obj_info;
struct pt_regs;
struct slab;

#define kfence_sample_interval	(0)

static inline bool is_kfence_address(const void *addr) { return false; }
static inline void kfence_alloc_pool_and_metadata(void) { }
static inline void kfence_init(void) { }
static inline void kfence_shutdown_cache(struct kmem_cache *s) { }
static inline void *kfence_alloc(struct kmem_cache *s, size_t size, gfp_t flags) { return NULL; }
static inline size_t kfence_ksize(const void *addr) { return 0; }
static inline void *kfence_object_start(const void *addr) { return NULL; }
static inline void __kfence_free(void *addr) { }
static inline bool __must_check kfence_free(void *addr) { return false; }
static inline bool __must_check kfence_handle_page_fault(unsigned long addr, bool is_write,
							 struct pt_regs *regs)
{
	return false;
}
static inline bool __kfence_obj_info(struct kmem_obj_info *kpp, void *object,
				     struct slab *slab)
{
	return false;
}

#endif /* _LINUX_KFENCE_H */
