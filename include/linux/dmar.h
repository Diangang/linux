/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2006, Intel Corporation.
 *
 * Copyright (C) Ashok Raj <ashok.raj@intel.com>
 * Copyright (C) Shaohua Li <shaohua.li@intel.com>
 */

#ifndef __DMAR_H__
#define __DMAR_H__

#include <linux/acpi.h>
#include <linux/types.h>
#include <linux/msi.h>
#include <linux/irqreturn.h>
#include <linux/rwsem.h>
#include <linux/rculist.h>

struct acpi_dmar_header;

#define DMAR_UNITS_SUPPORTED	1024

/* DMAR Flags */
#define DMAR_INTR_REMAP		0x1
#define DMAR_X2APIC_OPT_OUT	0x2
#define DMAR_PLATFORM_OPT_IN	0x4

struct intel_iommu;

struct dmar_dev_scope {
	struct device __rcu *dev;
	u8 bus;
	u8 devfn;
};


static inline int dmar_device_add(void *handle)
{
	return 0;
}

static inline int dmar_device_remove(void *handle)
{
	return 0;
}

static inline bool dmar_platform_optin(void)
{
	return false;
}

static inline void detect_intel_iommu(void)
{
}


struct irte {
	union {
		struct {
			union {
				/* Shared between remapped and posted mode*/
				struct {
					__u64	present		: 1,  /*  0      */
						fpd		: 1,  /*  1      */
						__res0		: 6,  /*  2 -  6 */
						avail		: 4,  /*  8 - 11 */
						__res1		: 3,  /* 12 - 14 */
						pst		: 1,  /* 15      */
						vector		: 8,  /* 16 - 23 */
						__res2		: 40; /* 24 - 63 */
				};

				/* Remapped mode */
				struct {
					__u64	r_present	: 1,  /*  0      */
						r_fpd		: 1,  /*  1      */
						dst_mode	: 1,  /*  2      */
						redir_hint	: 1,  /*  3      */
						trigger_mode	: 1,  /*  4      */
						dlvry_mode	: 3,  /*  5 -  7 */
						r_avail		: 4,  /*  8 - 11 */
						r_res0		: 4,  /* 12 - 15 */
						r_vector	: 8,  /* 16 - 23 */
						r_res1		: 8,  /* 24 - 31 */
						dest_id		: 32; /* 32 - 63 */
				};

				/* Posted mode */
				struct {
					__u64	p_present	: 1,  /*  0      */
						p_fpd		: 1,  /*  1      */
						p_res0		: 6,  /*  2 -  7 */
						p_avail		: 4,  /*  8 - 11 */
						p_res1		: 2,  /* 12 - 13 */
						p_urgent	: 1,  /* 14      */
						p_pst		: 1,  /* 15      */
						p_vector	: 8,  /* 16 - 23 */
						p_res2		: 14, /* 24 - 37 */
						pda_l		: 26; /* 38 - 63 */
				};
				__u64 low;
			};

			union {
				/* Shared between remapped and posted mode*/
				struct {
					__u64	sid		: 16,  /* 64 - 79  */
						sq		: 2,   /* 80 - 81  */
						svt		: 2,   /* 82 - 83  */
						__res3		: 44;  /* 84 - 127 */
				};

				/* Posted mode*/
				struct {
					__u64	p_sid		: 16,  /* 64 - 79  */
						p_sq		: 2,   /* 80 - 81  */
						p_svt		: 2,   /* 82 - 83  */
						p_res3		: 12,  /* 84 - 95  */
						pda_h		: 32;  /* 96 - 127 */
				};
				__u64 high;
			};
		};
	};
};

static inline void dmar_copy_shared_irte(struct irte *dst, struct irte *src)
{
	dst->present	= src->present;
	dst->fpd	= src->fpd;
	dst->avail	= src->avail;
	dst->pst	= src->pst;
	dst->vector	= src->vector;
	dst->sid	= src->sid;
	dst->sq		= src->sq;
	dst->svt	= src->svt;
}

#define PDA_LOW_BIT    26
#define PDA_HIGH_BIT   32

/* Can't use the common MSI interrupt functions
 * since DMAR is not a pci device
 */
struct irq_data;
extern void dmar_msi_unmask(struct irq_data *data);
extern void dmar_msi_mask(struct irq_data *data);
extern void dmar_msi_write(int irq, struct msi_msg *msg);
extern int dmar_set_interrupt(struct intel_iommu *iommu);
extern irqreturn_t dmar_fault(int irq, void *dev_id);
extern int dmar_alloc_hwirq(int id, int node, void *arg);
extern void dmar_free_hwirq(int irq);

#endif /* __DMAR_H__ */
