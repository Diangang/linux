/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_DNOTIFY_H
#define _LINUX_DNOTIFY_H
/*
 * Directory notification for Linux
 *
 * Copyright (C) 2000,2002 Stephen Rothwell
 */

#include <linux/fs.h>

struct dnotify_struct {
	struct dnotify_struct *	dn_next;
	__u32			dn_mask;
	int			dn_fd;
	struct file *		dn_filp;
	fl_owner_t		dn_owner;
};

#ifdef __KERNEL__


static inline void dnotify_flush(struct file *filp, fl_owner_t id)
{
}

static inline int fcntl_dirnotify(int fd, struct file *filp, unsigned int arg)
{
	return -EINVAL;
}

#endif /* __KERNEL __ */

#endif /* _LINUX_DNOTIFY_H */
