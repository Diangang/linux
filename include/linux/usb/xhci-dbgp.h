// SPDX-License-Identifier: GPL-2.0
/*
 * Standalone xHCI debug capability driver
 *
 * Copyright (C) 2016 Intel Corporation
 *
 * Author: Lu Baolu <baolu.lu@linux.intel.com>
 */

#ifndef __LINUX_XHCI_DBGP_H
#define __LINUX_XHCI_DBGP_H

static inline int __init early_xdbc_setup_hardware(void)
{
	return -ENODEV;
}
static inline void __init early_xdbc_register_console(void)
{
}
#endif /* __LINUX_XHCI_DBGP_H */
