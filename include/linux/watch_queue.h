// SPDX-License-Identifier: GPL-2.0
/* User-mappable watch queue
 *
 * Copyright (C) 2020 Red Hat, Inc. All Rights Reserved.
 * Written by David Howells (dhowells@redhat.com)
 *
 * See Documentation/core-api/watch_queue.rst
 */

#ifndef _LINUX_WATCH_QUEUE_H
#define _LINUX_WATCH_QUEUE_H

#include <uapi/linux/watch_queue.h>
#include <linux/kref.h>
#include <linux/rcupdate.h>

static inline int watch_queue_init(struct pipe_inode_info *pipe)
{
	return -ENOPKG;
}


#endif /* _LINUX_WATCH_QUEUE_H */
