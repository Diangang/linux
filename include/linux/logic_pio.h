// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2017 HiSilicon Limited, All Rights Reserved.
 * Author: Gabriele Paoloni <gabriele.paoloni@huawei.com>
 * Author: Zhichang Yuan <yuanzhichang@hisilicon.com>
 */

#ifndef __LINUX_LOGIC_PIO_H
#define __LINUX_LOGIC_PIO_H

#include <linux/fwnode.h>

struct logic_pio_hwaddr {
	struct list_head list;
	const struct fwnode_handle *fwnode;
	resource_size_t hw_start;
	resource_size_t io_start;
	resource_size_t size; /* range size populated */
};

#define MMIO_UPPER_LIMIT IO_SPACE_LIMIT

int logic_pio_register_range(struct logic_pio_hwaddr *newrange);
void logic_pio_unregister_range(struct logic_pio_hwaddr *range);
resource_size_t logic_pio_to_hwaddr(unsigned long pio);
unsigned long logic_pio_trans_cpuaddr(resource_size_t hw_addr);

#endif /* __LINUX_LOGIC_PIO_H */
