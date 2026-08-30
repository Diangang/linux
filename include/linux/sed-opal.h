/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright © 2016 Intel Corporation
 *
 * Authors:
 *    Rafael Antognolli <rafael.antognolli@intel.com>
 *    Scott  Bauer      <scott.bauer@intel.com>
 */

#ifndef LINUX_OPAL_H
#define LINUX_OPAL_H

#include <uapi/linux/sed-opal.h>
#include <linux/compiler_types.h>
#include <linux/types.h>

struct opal_dev;

typedef int (sec_send_recv)(void *data, u16 spsp, u8 secp, void *buffer,
		size_t len, bool send);

static inline void free_opal_dev(struct opal_dev *dev)
{
}

static inline bool is_sed_ioctl(unsigned int cmd)
{
	return false;
}

static inline int sed_ioctl(struct opal_dev *dev, unsigned int cmd,
			    void __user *ioctl_ptr)
{
	return 0;
}
static inline bool opal_unlock_from_suspend(struct opal_dev *dev)
{
	return false;
}
#define init_opal_dev(data, send_recv)		NULL
#endif /* LINUX_OPAL_H */
