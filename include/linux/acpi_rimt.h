/* SPDX-License-Identifier: GPL-2.0-only */
/*
 *  Copyright (C) 2024-2025, Ventana Micro Systems Inc.
 *	Author: Sunil V L <sunilvl@ventanamicro.com>
 */

#ifndef _ACPI_RIMT_H
#define _ACPI_RIMT_H

static inline int rimt_iommu_register(struct device *dev)
{
	return -ENODEV;
}

static inline int rimt_iommu_configure_id(struct device *dev, const u32 *id_in)
{
	return -ENODEV;
}

#endif /* _ACPI_RIMT_H */
