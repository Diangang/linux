/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2016 Parav Pandit <pandit.parav@gmail.com>
 */

#ifndef _CGROUP_RDMA_H
#define _CGROUP_RDMA_H

#include <linux/cgroup.h>

enum rdmacg_resource_type {
	RDMACG_RESOURCE_HCA_HANDLE,
	RDMACG_RESOURCE_HCA_OBJECT,
	RDMACG_RESOURCE_MAX,
};

struct rdma_cgroup;

struct rdmacg_device {
	struct list_head	dev_node;
	struct list_head	rpools;
	char			*name;
};

/*
 * APIs for RDMA/IB stack to publish when a device wants to
 * participate in resource accounting
 */
static inline void rdmacg_register_device(struct rdmacg_device *device) {}
static inline void rdmacg_unregister_device(struct rdmacg_device *device) {}

/* APIs for RDMA/IB stack to charge/uncharge pool specific resources */
static inline int rdmacg_try_charge(struct rdma_cgroup **rdmacg,
		      struct rdmacg_device *device,
		      enum rdmacg_resource_type index)
{
	return 0;
}
static inline void rdmacg_uncharge(struct rdma_cgroup *cg,
		     struct rdmacg_device *device,
		     enum rdmacg_resource_type index)
{
}
#endif	/* _CGROUP_RDMA_H */
