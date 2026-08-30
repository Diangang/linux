/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __ACPI_NUMA_H
#define __ACPI_NUMA_H

static inline int fix_pxm_node_maps(int max_nid)
{
	return 0;
}
static inline void disable_srat(void)
{
}
static inline int pxm_to_node(int pxm)
{
	return 0;
}
static inline int node_to_pxm(int node)
{
	return 0;
}

static inline void disable_hmat(void)
{
}
#endif				/* __ACPI_NUMA_H */
