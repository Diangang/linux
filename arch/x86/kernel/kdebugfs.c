// SPDX-License-Identifier: GPL-2.0-only
/*
 * Architecture specific debugfs files
 *
 * Copyright (C) 2007, Intel Corp.
 *	Huang Ying <ying.huang@intel.com>
 */
#include <linux/debugfs.h>
#include <linux/uaccess.h>
#include <linux/export.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/stat.h>
#include <linux/io.h>
#include <linux/mm.h>

#include <asm/setup.h>

struct dentry *arch_debugfs_dir;
EXPORT_SYMBOL(arch_debugfs_dir);


static int __init arch_kdebugfs_init(void)
{
	int error = 0;

	arch_debugfs_dir = debugfs_create_dir("x86", NULL);


	return error;
}
arch_initcall(arch_kdebugfs_init);
