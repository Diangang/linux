/* SPDX-License-Identifier: GPL-2.0-or-later */
/* -*- linux-c -*- --------------------------------------------------------- *
 *
 * linux/include/linux/devpts_fs.h
 *
 *  Copyright 1998-2004 H. Peter Anvin -- All Rights Reserved
 *
 * ------------------------------------------------------------------------- */

#ifndef _LINUX_DEVPTS_FS_H
#define _LINUX_DEVPTS_FS_H

#include <linux/errno.h>

static inline int
ptm_open_peer(struct file *master, struct tty_struct *tty, int flags)
{
	return -EIO;
}


#endif /* _LINUX_DEVPTS_FS_H */
