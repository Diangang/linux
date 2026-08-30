/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 *  include/linux/eventpoll.h ( Efficient event polling implementation )
 *  Copyright (C) 2001,...,2006	 Davide Libenzi
 *
 *  Davide Libenzi <davidel@xmailserver.org>
 */
#ifndef _LINUX_EVENTPOLL_H
#define _LINUX_EVENTPOLL_H

#include <uapi/linux/eventpoll.h>
#include <uapi/linux/kcmp.h>


/* Forward declarations to avoid compiler errors */
struct file;



static inline void eventpoll_release(struct file *file) {}


static inline struct epoll_event __user *
epoll_put_uevent(__poll_t revents, __u64 data,
		 struct epoll_event __user *uevent)
{
	scoped_user_write_access_size(uevent, sizeof(*uevent), efault) {
		unsafe_put_user(revents, &uevent->events, efault);
		unsafe_put_user(data, &uevent->data, efault);
	}
	return uevent+1;

efault:
	return NULL;
}

#endif /* #ifndef _LINUX_EVENTPOLL_H */
