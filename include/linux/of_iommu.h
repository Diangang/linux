/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __OF_IOMMU_H
#define __OF_IOMMU_H

struct device;
struct device_node;
struct iommu_ops;


static inline int of_iommu_configure(struct device *dev,
				     struct device_node *master_np,
				     const u32 *id)
{
	return -ENODEV;
}

static inline void of_iommu_get_resv_regions(struct device *dev,
					     struct list_head *list)
{
}


#endif /* __OF_IOMMU_H */
