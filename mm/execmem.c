// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2002 Richard Henderson
 * Copyright (C) 2001 Rusty Russell, 2002, 2010 Rusty Russell IBM.
 * Copyright (C) 2023 Luis Chamberlain <mcgrof@kernel.org>
 * Copyright (C) 2024 Mike Rapoport IBM.
 */

#define pr_fmt(fmt) "execmem: " fmt

#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/execmem.h>

#include "internal.h"

static struct execmem_info *execmem_info __ro_after_init;
static struct execmem_info default_execmem_info __ro_after_init;

#ifdef CONFIG_MMU
static void *execmem_vmalloc(struct execmem_range *range, size_t size,
			     pgprot_t pgprot, unsigned long vm_flags)
{
	gfp_t gfp_flags = GFP_KERNEL | __GFP_NOWARN;
	unsigned int align = range->alignment;
	unsigned long start = range->start;
	unsigned long end = range->end;
	void *p;

	p = __vmalloc_node_range(size, align, start, end, gfp_flags,
				 pgprot, vm_flags, NUMA_NO_NODE,
				 __builtin_return_address(0));
	if (!p && range->fallback_start) {
		start = range->fallback_start;
		end = range->fallback_end;
		p = __vmalloc_node_range(size, align, start, end, gfp_flags,
					 pgprot, vm_flags, NUMA_NO_NODE,
					 __builtin_return_address(0));
	}

	if (!p) {
		pr_warn_ratelimited("unable to allocate memory\n");
		return NULL;
	}

	return p;
}

struct vm_struct *execmem_vmap(size_t size)
{
	struct execmem_range *range = &execmem_info->ranges[EXECMEM_MODULE_DATA];
	struct vm_struct *area;

	area = __get_vm_area_node(size, range->alignment, PAGE_SHIFT, VM_ALLOC,
				  range->start, range->end, NUMA_NO_NODE,
				  GFP_KERNEL, __builtin_return_address(0));
	if (!area && range->fallback_start)
		area = __get_vm_area_node(size, range->alignment, PAGE_SHIFT, VM_ALLOC,
					  range->fallback_start, range->fallback_end,
					  NUMA_NO_NODE, GFP_KERNEL, __builtin_return_address(0));

	return area;
}
#else
static void *execmem_vmalloc(struct execmem_range *range, size_t size,
			     pgprot_t pgprot, unsigned long vm_flags)
{
	return vmalloc(size);
}
#endif /* CONFIG_MMU */


void *execmem_alloc(enum execmem_type type, size_t size)
{
	struct execmem_range *range = &execmem_info->ranges[type];
	unsigned long vm_flags = VM_FLUSH_RESET_PERMS;
	pgprot_t pgprot = range->pgprot;

	size = PAGE_ALIGN(size);

	return execmem_vmalloc(range, size, pgprot, vm_flags);
}

void *execmem_alloc_rw(enum execmem_type type, size_t size)
{
	return execmem_alloc(type, size);
}

void execmem_free(void *ptr)
{
	WARN_ON(in_interrupt());

	vfree(ptr);
}

static bool execmem_validate(struct execmem_info *info)
{
	struct execmem_range *r = &info->ranges[EXECMEM_DEFAULT];

	if (!r->alignment || !r->start || !r->end || !pgprot_val(r->pgprot)) {
		pr_crit("Invalid parameters for execmem allocator, module loading will fail");
		return false;
	}

	return true;
}

static void execmem_init_missing(struct execmem_info *info)
{
	struct execmem_range *default_range = &info->ranges[EXECMEM_DEFAULT];

	for (int i = EXECMEM_DEFAULT + 1; i < EXECMEM_TYPE_MAX; i++) {
		struct execmem_range *r = &info->ranges[i];

		if (!r->start) {
			if (i == EXECMEM_MODULE_DATA)
				r->pgprot = PAGE_KERNEL;
			else
				r->pgprot = default_range->pgprot;
			r->alignment = default_range->alignment;
			r->start = default_range->start;
			r->end = default_range->end;
			r->fallback_start = default_range->fallback_start;
			r->fallback_end = default_range->fallback_end;
		}
	}
}

struct execmem_info * __weak execmem_arch_setup(void)
{
	return NULL;
}

static void __init __execmem_init(void)
{
	struct execmem_info *info = execmem_arch_setup();

	if (!info) {
		info = execmem_info = &default_execmem_info;
		info->ranges[EXECMEM_DEFAULT].start = VMALLOC_START;
		info->ranges[EXECMEM_DEFAULT].end = VMALLOC_END;
		info->ranges[EXECMEM_DEFAULT].pgprot = PAGE_KERNEL_EXEC;
		info->ranges[EXECMEM_DEFAULT].alignment = 1;
	}

	if (!execmem_validate(info))
		return;

	execmem_init_missing(info);

	execmem_info = info;
}

#ifdef CONFIG_ARCH_WANTS_EXECMEM_LATE
static int __init execmem_late_init(void)
{
	__execmem_init();
	return 0;
}
core_initcall(execmem_late_init);
#else
void __init execmem_init(void)
{
	__execmem_init();
}
#endif
