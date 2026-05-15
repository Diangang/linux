// SPDX-License-Identifier: GPL-2.0-only
/*
 * nvs.c - Routines for saving and restoring ACPI NVS memory region
 *
 * Copyright (C) 2008-2011 Rafael J. Wysocki <rjw@sisk.pl>, Novell Inc.
 */

#define pr_fmt(fmt) "ACPI: PM: " fmt

#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/acpi.h>

#include "internal.h"

/* ACPI NVS regions, APEI may use it */

struct nvs_region {
	__u64 phys_start;
	__u64 size;
	struct list_head node;
};

static LIST_HEAD(nvs_region_list);

int acpi_nvs_register(__u64 start, __u64 size)
{
	struct nvs_region *region;

	region = kmalloc_obj(*region);
	if (!region)
		return -ENOMEM;
	region->phys_start = start;
	region->size = size;
	list_add_tail(&region->node, &nvs_region_list);

	return 0;
}

int acpi_nvs_for_each_region(int (*func)(__u64 start, __u64 size, void *data),
			     void *data)
{
	int rc;
	struct nvs_region *region;

	list_for_each_entry(region, &nvs_region_list, node) {
		rc = func(region->phys_start, region->size, data);
		if (rc)
			return rc;
	}

	return 0;
}
