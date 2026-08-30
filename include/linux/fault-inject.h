/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_FAULT_INJECT_H
#define _LINUX_FAULT_INJECT_H

#include <linux/err.h>
#include <linux/types.h>

struct dentry;
struct kmem_cache;

enum fault_flags {
	FAULT_NOWARN =	1 << 0,
};


struct fault_attr {
};

#define DECLARE_FAULT_ATTR(name) struct fault_attr name = {}

static inline int setup_fault_attr(struct fault_attr *attr, char *str)
{
	return 0; /* Note: 0 means error for __setup() handlers! */
}
static inline bool should_fail_ex(struct fault_attr *attr, ssize_t size, int flags)
{
	return false;
}
static inline bool should_fail(struct fault_attr *attr, ssize_t size)
{
	return false;
}



static inline struct dentry *fault_create_debugfs_attr(const char *name,
			struct dentry *parent, struct fault_attr *attr)
{
	return ERR_PTR(-ENODEV);
}



struct fault_config {
};

static inline void fault_config_init(struct fault_config *config,
			const char *name)
{
}


static inline bool should_fail_alloc_page(gfp_t gfp_mask, unsigned int order)
{
	return false;
}

static inline int should_failslab(struct kmem_cache *s, gfp_t gfpflags)
{
	return false;
}

#endif /* _LINUX_FAULT_INJECT_H */
