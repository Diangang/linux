/* SPDX-License-Identifier: GPL-2.0 */
/*
 * /proc/kcore definitions
 */
#ifndef _LINUX_KCORE_H
#define _LINUX_KCORE_H

enum kcore_type {
	KCORE_TEXT,
	KCORE_VMALLOC,
	KCORE_RAM,
	KCORE_VMEMMAP,
	KCORE_USER,
};

struct kcore_list {
	struct list_head list;
	unsigned long addr;
	size_t size;
	int type;
};

static inline
void kclist_add(struct kcore_list *new, void *addr, size_t size, int type)
{
}

#endif /* _LINUX_KCORE_H */
