/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef _LINUX_SYSFB_H
#define _LINUX_SYSFB_H

/*
 * Generic System Framebuffers on x86
 * Copyright (c) 2012-2013 David Herrmann <dh.herrmann@gmail.com>
 */

#include <linux/screen_info.h>


struct device;
struct screen_info;

struct sysfb_display_info {
	struct screen_info screen;

};

extern struct sysfb_display_info sysfb_primary_display;

static inline void sysfb_disable(struct device *dev)
{
}

#endif /* _LINUX_SYSFB_H */
