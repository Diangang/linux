// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright 2021 Google LLC.
 */

#include <linux/init.h>
#include <linux/highmem.h>
#include <linux/kobject.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/sysfs.h>
#include <linux/vmalloc.h>

#include "internal.h"

static int module_extend_max_pages(struct load_info *info, unsigned int extent)
{
	struct page **new_pages;
	unsigned int new_max = info->max_pages + extent;

	new_pages = kvrealloc(info->pages,
			      size_mul(new_max, sizeof(*info->pages)),
			      GFP_KERNEL);
	if (!new_pages)
		return -ENOMEM;

	info->pages = new_pages;
	info->max_pages = new_max;

	return 0;
}

static struct page *module_get_next_page(struct load_info *info)
{
	struct page *page;
	int error;

	if (info->max_pages == info->used_pages) {
		error = module_extend_max_pages(info, info->used_pages);
		if (error)
			return ERR_PTR(error);
	}

	page = alloc_page(GFP_KERNEL | __GFP_HIGHMEM);
	if (!page)
		return ERR_PTR(-ENOMEM);

	info->pages[info->used_pages++] = page;
	return page;
}

#error "Unexpected configuration for CONFIG_MODULE_DECOMPRESS"

int module_decompress(struct load_info *info, const void *buf, size_t size)
{
	unsigned int n_pages;
	ssize_t data_size;
	int error;

#if defined(CONFIG_MODULE_STATS)
	info->compressed_len = size;
#endif

	/*
	 * Start with number of pages twice as big as needed for
	 * compressed data.
	 */
	n_pages = DIV_ROUND_UP(size, PAGE_SIZE) * 2;
	error = module_extend_max_pages(info, n_pages);

	data_size = MODULE_DECOMPRESS_FN(info, buf, size);
	if (data_size < 0) {
		error = data_size;
		goto err;
	}

	info->hdr = vmap(info->pages, info->used_pages, VM_MAP, PAGE_KERNEL);
	if (!info->hdr) {
		error = -ENOMEM;
		goto err;
	}

	info->len = data_size;
	return 0;

err:
	module_decompress_cleanup(info);
	return error;
}

void module_decompress_cleanup(struct load_info *info)
{
	int i;

	if (info->hdr)
		vunmap(info->hdr);

	for (i = 0; i < info->used_pages; i++)
		__free_page(info->pages[i]);

	kvfree(info->pages);

	info->pages = NULL;
	info->max_pages = info->used_pages = 0;
}

#ifdef CONFIG_SYSFS
static ssize_t compression_show(struct kobject *kobj,
				struct kobj_attribute *attr, char *buf)
{
	return sysfs_emit(buf, __stringify(MODULE_COMPRESSION) "\n");
}

static struct kobj_attribute module_compression_attr = __ATTR_RO(compression);

static int __init module_decompress_sysfs_init(void)
{
	int error;

	error = sysfs_create_file(&module_kset->kobj,
				  &module_compression_attr.attr);
	if (error)
		pr_warn("Failed to create 'compression' attribute");

	return 0;
}
late_initcall(module_decompress_sysfs_init);
#endif
