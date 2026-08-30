/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2007-2008 Advanced Micro Devices, Inc.
 * Author: Joerg Roedel <joerg.roedel@amd.com>
 */

#ifndef __LINUX_IOMMU_H
#define __LINUX_IOMMU_H

#include <linux/scatterlist.h>
#include <linux/device.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/of.h>
#include <linux/iova_bitmap.h>
#include <uapi/linux/iommufd.h>

#define IOMMU_READ	(1 << 0)
#define IOMMU_WRITE	(1 << 1)
#define IOMMU_CACHE	(1 << 2) /* DMA cache coherency */
#define IOMMU_NOEXEC	(1 << 3)
#define IOMMU_MMIO	(1 << 4) /* e.g. things like MSI doorbells */
/*
 * Where the bus hardware includes a privilege level as part of its access type
 * markings, and certain devices are capable of issuing transactions marked as
 * either 'supervisor' or 'user', the IOMMU_PRIV flag requests that the other
 * given permission flags only apply to accesses at the higher privilege level,
 * and that unprivileged transactions should have as little access as possible.
 * This would usually imply the same permissions as kernel mappings on the CPU,
 * if the IOMMU page table format is equivalent.
 */
#define IOMMU_PRIV	(1 << 5)

struct iommu_ops;
struct iommu_group;
struct bus_type;
struct device;
struct iommu_domain;
struct iommu_domain_ops;
struct iommu_dirty_ops;
struct notifier_block;
struct iommu_sva;
struct iommu_dma_cookie;
struct iommu_dma_msi_cookie;
struct iommu_fault_param;
struct iommufd_ctx;
struct iommufd_viommu;
struct msi_desc;
struct msi_msg;

#define IOMMU_FAULT_PERM_READ	(1 << 0) /* read */
#define IOMMU_FAULT_PERM_WRITE	(1 << 1) /* write */
#define IOMMU_FAULT_PERM_EXEC	(1 << 2) /* exec */
#define IOMMU_FAULT_PERM_PRIV	(1 << 3) /* privileged */

/* Generic fault types, can be expanded IRQ remapping fault */
enum iommu_fault_type {
	IOMMU_FAULT_PAGE_REQ = 1,	/* page request fault */
};

/**
 * struct iommu_fault_page_request - Page Request data
 * @flags: encodes whether the corresponding fields are valid and whether this
 *         is the last page in group (IOMMU_FAULT_PAGE_REQUEST_* values).
 *         When IOMMU_FAULT_PAGE_RESPONSE_NEEDS_PASID is set, the page response
 *         must have the same PASID value as the page request. When it is clear,
 *         the page response should not have a PASID.
 * @pasid: Process Address Space ID
 * @grpid: Page Request Group Index
 * @perm: requested page permissions (IOMMU_FAULT_PERM_* values)
 * @addr: page address
 * @private_data: device-specific private information
 */
struct iommu_fault_page_request {
#define IOMMU_FAULT_PAGE_REQUEST_PASID_VALID	(1 << 0)
#define IOMMU_FAULT_PAGE_REQUEST_LAST_PAGE	(1 << 1)
#define IOMMU_FAULT_PAGE_RESPONSE_NEEDS_PASID	(1 << 2)
	u32	flags;
	u32	pasid;
	u32	grpid;
	u32	perm;
	u64	addr;
	u64	private_data[2];
};

/**
 * struct iommu_fault - Generic fault data
 * @type: fault type from &enum iommu_fault_type
 * @prm: Page Request message, when @type is %IOMMU_FAULT_PAGE_REQ
 */
struct iommu_fault {
	u32 type;
	struct iommu_fault_page_request prm;
};

/**
 * enum iommu_page_response_code - Return status of fault handlers
 * @IOMMU_PAGE_RESP_SUCCESS: Fault has been handled and the page tables
 *	populated, retry the access. This is "Success" in PCI PRI.
 * @IOMMU_PAGE_RESP_FAILURE: General error. Drop all subsequent faults from
 *	this device if possible. This is "Response Failure" in PCI PRI.
 * @IOMMU_PAGE_RESP_INVALID: Could not handle this fault, don't retry the
 *	access. This is "Invalid Request" in PCI PRI.
 */
enum iommu_page_response_code {
	IOMMU_PAGE_RESP_SUCCESS = 0,
	IOMMU_PAGE_RESP_INVALID,
	IOMMU_PAGE_RESP_FAILURE,
};

/**
 * struct iommu_page_response - Generic page response information
 * @pasid: Process Address Space ID
 * @grpid: Page Request Group Index
 * @code: response code from &enum iommu_page_response_code
 */
struct iommu_page_response {
	u32	pasid;
	u32	grpid;
	u32	code;
};

struct iopf_fault {
	struct iommu_fault fault;
	/* node for pending lists */
	struct list_head list;
};

struct iopf_group {
	struct iopf_fault last_fault;
	struct list_head faults;
	size_t fault_count;
	/* list node for iommu_fault_param::faults */
	struct list_head pending_node;
	struct work_struct work;
	struct iommu_attach_handle *attach_handle;
	/* The device's fault data parameter. */
	struct iommu_fault_param *fault_param;
	/* Used by handler provider to hook the group on its own lists. */
	struct list_head node;
	u32 cookie;
};

/**
 * struct iopf_queue - IO Page Fault queue
 * @wq: the fault workqueue
 * @devices: devices attached to this queue
 * @lock: protects the device list
 */
struct iopf_queue {
	struct workqueue_struct *wq;
	struct list_head devices;
	struct mutex lock;
};

/* iommu fault flags */
#define IOMMU_FAULT_READ	0x0
#define IOMMU_FAULT_WRITE	0x1

typedef int (*iommu_fault_handler_t)(struct iommu_domain *,
			struct device *, unsigned long, int, void *);

struct iommu_domain_geometry {
	dma_addr_t aperture_start; /* First address that can be mapped    */
	dma_addr_t aperture_end;   /* Last address that can be mapped     */
	bool force_aperture;       /* DMA only allowed in mappable range? */
};

enum iommu_domain_cookie_type {
	IOMMU_COOKIE_NONE,
	IOMMU_COOKIE_DMA_IOVA,
	IOMMU_COOKIE_DMA_MSI,
	IOMMU_COOKIE_FAULT_HANDLER,
	IOMMU_COOKIE_SVA,
	IOMMU_COOKIE_IOMMUFD,
};

/* Domain feature flags */
#define __IOMMU_DOMAIN_PAGING	(1U << 0)  /* Support for iommu_map/unmap */
#define __IOMMU_DOMAIN_DMA_API	(1U << 1)  /* Domain for use in DMA-API
					      implementation              */
#define __IOMMU_DOMAIN_PT	(1U << 2)  /* Domain is identity mapped   */
#define __IOMMU_DOMAIN_DMA_FQ	(1U << 3)  /* DMA-API uses flush queue    */

#define __IOMMU_DOMAIN_SVA	(1U << 4)  /* Shared process address space */
#define __IOMMU_DOMAIN_PLATFORM	(1U << 5)

#define __IOMMU_DOMAIN_NESTED	(1U << 6)  /* User-managed address space nested
					      on a stage-2 translation        */

#define IOMMU_DOMAIN_ALLOC_FLAGS ~__IOMMU_DOMAIN_DMA_FQ
/*
 * This are the possible domain-types
 *
 *	IOMMU_DOMAIN_BLOCKED	- All DMA is blocked, can be used to isolate
 *				  devices
 *	IOMMU_DOMAIN_IDENTITY	- DMA addresses are system physical addresses
 *	IOMMU_DOMAIN_UNMANAGED	- DMA mappings managed by IOMMU-API user, used
 *				  for VMs
 *	IOMMU_DOMAIN_DMA	- Internally used for DMA-API implementations.
 *				  This flag allows IOMMU drivers to implement
 *				  certain optimizations for these domains
 *	IOMMU_DOMAIN_DMA_FQ	- As above, but definitely using batched TLB
 *				  invalidation.
 *	IOMMU_DOMAIN_SVA	- DMA addresses are shared process addresses
 *				  represented by mm_struct's.
 *	IOMMU_DOMAIN_PLATFORM	- Legacy domain for drivers that do their own
 *				  dma_api stuff. Do not use in new drivers.
 */
#define IOMMU_DOMAIN_BLOCKED	(0U)
#define IOMMU_DOMAIN_IDENTITY	(__IOMMU_DOMAIN_PT)
#define IOMMU_DOMAIN_UNMANAGED	(__IOMMU_DOMAIN_PAGING)
#define IOMMU_DOMAIN_DMA	(__IOMMU_DOMAIN_PAGING |	\
				 __IOMMU_DOMAIN_DMA_API)
#define IOMMU_DOMAIN_DMA_FQ	(__IOMMU_DOMAIN_PAGING |	\
				 __IOMMU_DOMAIN_DMA_API |	\
				 __IOMMU_DOMAIN_DMA_FQ)
#define IOMMU_DOMAIN_SVA	(__IOMMU_DOMAIN_SVA)
#define IOMMU_DOMAIN_PLATFORM	(__IOMMU_DOMAIN_PLATFORM)
#define IOMMU_DOMAIN_NESTED	(__IOMMU_DOMAIN_NESTED)

struct iommu_domain {
	unsigned type;
	enum iommu_domain_cookie_type cookie_type;
	bool is_iommupt;
	const struct iommu_domain_ops *ops;
	const struct iommu_dirty_ops *dirty_ops;
	const struct iommu_ops *owner; /* Whose domain_alloc we came from */
	unsigned long pgsize_bitmap;	/* Bitmap of page sizes in use */
	struct iommu_domain_geometry geometry;
	int (*iopf_handler)(struct iopf_group *group);

	union { /* cookie */
		struct iommu_dma_cookie *iova_cookie;
		struct iommu_dma_msi_cookie *msi_cookie;
		struct iommufd_hw_pagetable *iommufd_hwpt;
		struct {
			iommu_fault_handler_t handler;
			void *handler_token;
		};
		struct {	/* IOMMU_DOMAIN_SVA */
			struct mm_struct *mm;
			int users;
			/*
			 * Next iommu_domain in mm->iommu_mm->sva-domains list
			 * protected by iommu_sva_lock.
			 */
			struct list_head next;
		};
	};
};

static inline bool iommu_is_dma_domain(struct iommu_domain *domain)
{
	return domain->type & __IOMMU_DOMAIN_DMA_API;
}

enum iommu_cap {
	IOMMU_CAP_CACHE_COHERENCY,	/* IOMMU_CACHE is supported */
	IOMMU_CAP_NOEXEC,		/* IOMMU_NOEXEC flag */
	IOMMU_CAP_PRE_BOOT_PROTECTION,	/* Firmware says it used the IOMMU for
					   DMA protection and we should too */
	/*
	 * Per-device flag indicating if enforce_cache_coherency() will work on
	 * this device.
	 */
	IOMMU_CAP_ENFORCE_CACHE_COHERENCY,
	/*
	 * IOMMU driver does not issue TLB maintenance during .unmap, so can
	 * usefully support the non-strict DMA flush queue.
	 */
	IOMMU_CAP_DEFERRED_FLUSH,
	IOMMU_CAP_DIRTY_TRACKING,	/* IOMMU supports dirty tracking */
	/* ATS is supported and may be enabled for this device */
	IOMMU_CAP_PCI_ATS_SUPPORTED,
};

/* These are the possible reserved region types */
enum iommu_resv_type {
	/* Memory regions which must be mapped 1:1 at all times */
	IOMMU_RESV_DIRECT,
	/*
	 * Memory regions which are advertised to be 1:1 but are
	 * commonly considered relaxable in some conditions,
	 * for instance in device assignment use case (USB, Graphics)
	 */
	IOMMU_RESV_DIRECT_RELAXABLE,
	/* Arbitrary "never map this or give it to a device" address ranges */
	IOMMU_RESV_RESERVED,
	/* Hardware MSI region (untranslated) */
	IOMMU_RESV_MSI,
	/* Software-managed MSI translation window */
	IOMMU_RESV_SW_MSI,
};

/**
 * struct iommu_resv_region - descriptor for a reserved memory region
 * @list: Linked list pointers
 * @start: System physical start address of the region
 * @length: Length of the region in bytes
 * @prot: IOMMU Protection flags (READ/WRITE/...)
 * @type: Type of the reserved region
 * @free: Callback to free associated memory allocations
 */
struct iommu_resv_region {
	struct list_head	list;
	phys_addr_t		start;
	size_t			length;
	int			prot;
	enum iommu_resv_type	type;
	void (*free)(struct device *dev, struct iommu_resv_region *region);
};

struct iommu_iort_rmr_data {
	struct iommu_resv_region rr;

	/* Stream IDs associated with IORT RMR entry */
	const u32 *sids;
	u32 num_sids;
};

#define IOMMU_NO_PASID	(0U) /* Reserved for DMA w/o PASID */
#define IOMMU_FIRST_GLOBAL_PASID	(1U) /*starting range for allocation */
#define IOMMU_PASID_INVALID	(-1U)
typedef unsigned int ioasid_t;

/* Read but do not clear any dirty bits */
#define IOMMU_DIRTY_NO_CLEAR (1 << 0)

/*
 * Pages allocated through iommu_alloc_pages_node_sz() can be placed on this
 * list using iommu_pages_list_add(). Note: ONLY pages from
 * iommu_alloc_pages_node_sz() can be used this way!
 */
struct iommu_pages_list {
	struct list_head pages;
};

#define IOMMU_PAGES_LIST_INIT(name) \
	((struct iommu_pages_list){ .pages = LIST_HEAD_INIT(name.pages) })


struct iommu_ops {};
struct iommu_group {};
struct iommu_fwspec {};
struct iommu_device {};
struct iommu_fault_param {};
struct iommu_iotlb_gather {};
struct iommu_dirty_bitmap {};
struct iommu_dirty_ops {};

static inline bool device_iommu_capable(struct device *dev, enum iommu_cap cap)
{
	return false;
}

static inline struct iommu_domain *iommu_paging_domain_alloc_flags(struct device *dev,
						     unsigned int flags)
{
	return ERR_PTR(-ENODEV);
}

static inline struct iommu_domain *iommu_paging_domain_alloc(struct device *dev)
{
	return ERR_PTR(-ENODEV);
}

static inline void iommu_domain_free(struct iommu_domain *domain)
{
}

static inline int iommu_attach_device(struct iommu_domain *domain,
				      struct device *dev)
{
	return -ENODEV;
}

static inline void iommu_detach_device(struct iommu_domain *domain,
				       struct device *dev)
{
}

static inline struct iommu_domain *iommu_get_domain_for_dev(struct device *dev)
{
	return NULL;
}

static inline int iommu_map(struct iommu_domain *domain, unsigned long iova,
			    phys_addr_t paddr, size_t size, int prot, gfp_t gfp)
{
	return -ENODEV;
}

static inline size_t iommu_unmap(struct iommu_domain *domain,
				 unsigned long iova, size_t size)
{
	return 0;
}

static inline size_t iommu_unmap_fast(struct iommu_domain *domain,
				      unsigned long iova, int gfp_order,
				      struct iommu_iotlb_gather *iotlb_gather)
{
	return 0;
}

static inline ssize_t iommu_map_sg(struct iommu_domain *domain,
				   unsigned long iova, struct scatterlist *sg,
				   unsigned int nents, int prot, gfp_t gfp)
{
	return -ENODEV;
}

static inline void iommu_flush_iotlb_all(struct iommu_domain *domain)
{
}

static inline void iommu_iotlb_sync(struct iommu_domain *domain,
				  struct iommu_iotlb_gather *iotlb_gather)
{
}

static inline phys_addr_t iommu_iova_to_phys(struct iommu_domain *domain, dma_addr_t iova)
{
	return 0;
}

static inline void iommu_set_fault_handler(struct iommu_domain *domain,
				iommu_fault_handler_t handler, void *token)
{
}

static inline void iommu_get_resv_regions(struct device *dev,
					struct list_head *list)
{
}

static inline void iommu_put_resv_regions(struct device *dev,
					struct list_head *list)
{
}

static inline int iommu_get_group_resv_regions(struct iommu_group *group,
					       struct list_head *head)
{
	return -ENODEV;
}

static inline void iommu_set_default_passthrough(bool cmd_line)
{
}

static inline void iommu_set_default_translated(bool cmd_line)
{
}

static inline bool iommu_default_passthrough(void)
{
	return true;
}

static inline int iommu_attach_group(struct iommu_domain *domain,
				     struct iommu_group *group)
{
	return -ENODEV;
}

static inline void iommu_detach_group(struct iommu_domain *domain,
				      struct iommu_group *group)
{
}

static inline struct iommu_group *iommu_group_alloc(void)
{
	return ERR_PTR(-ENODEV);
}

static inline void *iommu_group_get_iommudata(struct iommu_group *group)
{
	return NULL;
}

static inline void iommu_group_set_iommudata(struct iommu_group *group,
					     void *iommu_data,
					     void (*release)(void *iommu_data))
{
}

static inline int iommu_group_set_name(struct iommu_group *group,
				       const char *name)
{
	return -ENODEV;
}

static inline int iommu_group_add_device(struct iommu_group *group,
					 struct device *dev)
{
	return -ENODEV;
}

static inline void iommu_group_remove_device(struct device *dev)
{
}

static inline int iommu_group_for_each_dev(struct iommu_group *group,
					   void *data,
					   int (*fn)(struct device *, void *))
{
	return -ENODEV;
}

static inline struct iommu_group *iommu_group_get(struct device *dev)
{
	return NULL;
}

static inline void iommu_group_put(struct iommu_group *group)
{
}

static inline int iommu_group_id(struct iommu_group *group)
{
	return -ENODEV;
}

static inline int iommu_set_pgtable_quirks(struct iommu_domain *domain,
		unsigned long quirks)
{
	return 0;
}

static inline int iommu_device_register(struct iommu_device *iommu,
					const struct iommu_ops *ops,
					struct device *hwdev)
{
	return -ENODEV;
}

static inline struct iommu_device *dev_to_iommu_device(struct device *dev)
{
	return NULL;
}

static inline void iommu_iotlb_gather_init(struct iommu_iotlb_gather *gather)
{
}

static inline void iommu_iotlb_gather_add_page(struct iommu_domain *domain,
					       struct iommu_iotlb_gather *gather,
					       unsigned long iova, size_t size)
{
}

static inline bool iommu_iotlb_gather_queued(struct iommu_iotlb_gather *gather)
{
	return false;
}

static inline void iommu_dirty_bitmap_init(struct iommu_dirty_bitmap *dirty,
					   struct iova_bitmap *bitmap,
					   struct iommu_iotlb_gather *gather)
{
}

static inline void iommu_dirty_bitmap_record(struct iommu_dirty_bitmap *dirty,
					     unsigned long iova,
					     unsigned long length)
{
}

static inline void iommu_device_unregister(struct iommu_device *iommu)
{
}

static inline int  iommu_device_sysfs_add(struct iommu_device *iommu,
					  struct device *parent,
					  const struct attribute_group **groups,
					  const char *fmt, ...)
{
	return -ENODEV;
}

static inline void iommu_device_sysfs_remove(struct iommu_device *iommu)
{
}

static inline int iommu_device_link(struct device *dev, struct device *link)
{
	return -EINVAL;
}

static inline void iommu_device_unlink(struct device *dev, struct device *link)
{
}

static inline int iommu_fwspec_init(struct device *dev,
				    struct fwnode_handle *iommu_fwnode)
{
	return -ENODEV;
}

static inline int iommu_fwspec_add_ids(struct device *dev, u32 *ids,
				       int num_ids)
{
	return -ENODEV;
}

static inline struct iommu_fwspec *dev_iommu_fwspec_get(struct device *dev)
{
	return NULL;
}

static inline int iommu_device_use_default_domain(struct device *dev)
{
	return 0;
}

static inline void iommu_device_unuse_default_domain(struct device *dev)
{
}

static inline int
iommu_group_claim_dma_owner(struct iommu_group *group, void *owner)
{
	return -ENODEV;
}

static inline void iommu_group_release_dma_owner(struct iommu_group *group)
{
}

static inline bool iommu_group_dma_owner_claimed(struct iommu_group *group)
{
	return false;
}

static inline void iommu_device_release_dma_owner(struct device *dev)
{
}

static inline int iommu_device_claim_dma_owner(struct device *dev, void *owner)
{
	return -ENODEV;
}

static inline int iommu_attach_device_pasid(struct iommu_domain *domain,
					    struct device *dev, ioasid_t pasid,
					    struct iommu_attach_handle *handle)
{
	return -ENODEV;
}

static inline void iommu_detach_device_pasid(struct iommu_domain *domain,
					     struct device *dev, ioasid_t pasid)
{
}

static inline ioasid_t iommu_alloc_global_pasid(struct device *dev)
{
	return IOMMU_PASID_INVALID;
}

static inline void iommu_free_global_pasid(ioasid_t pasid) {}

static inline int pci_dev_reset_iommu_prepare(struct pci_dev *pdev)
{
	return 0;
}

static inline void pci_dev_reset_iommu_done(struct pci_dev *pdev)
{
}

#ifdef CONFIG_IRQ_MSI_IOMMU
static inline int iommu_dma_prepare_msi(struct msi_desc *desc,
					phys_addr_t msi_addr)
{
	return 0;
}
#endif /* CONFIG_IRQ_MSI_IOMMU */

static inline void iommu_group_mutex_assert(struct device *dev)
{
}

/**
 * iommu_map_sgtable - Map the given buffer to the IOMMU domain
 * @domain:	The IOMMU domain to perform the mapping
 * @iova:	The start address to map the buffer
 * @sgt:	The sg_table object describing the buffer
 * @prot:	IOMMU protection bits
 *
 * Creates a mapping at @iova for the buffer described by a scatterlist
 * stored in the given sg_table object in the provided IOMMU domain.
 */
static inline ssize_t iommu_map_sgtable(struct iommu_domain *domain,
			unsigned long iova, struct sg_table *sgt, int prot)
{
	return iommu_map_sg(domain, iova, sgt->sgl, sgt->orig_nents, prot,
			    GFP_KERNEL);
}

static inline void iommu_debugfs_setup(void) {}

static inline int iommu_get_msi_cookie(struct iommu_domain *domain, dma_addr_t base)
{
	return -ENODEV;
}

/*
 * Newer generations of Tegra SoCs require devices' stream IDs to be directly programmed into
 * some registers. These are always paired with a Tegra SMMU or ARM SMMU, for which the contents
 * of the struct iommu_fwspec are known. Use this helper to formalize access to these internals.
 */
#define TEGRA_STREAM_ID_BYPASS 0x7f

static inline bool tegra_dev_iommu_get_stream_id(struct device *dev, u32 *stream_id)
{

	return false;
}

static inline struct iommu_sva *
iommu_sva_bind_device(struct device *dev, struct mm_struct *mm)
{
	return ERR_PTR(-ENODEV);
}

static inline void iommu_sva_unbind_device(struct iommu_sva *handle)
{
}

static inline u32 iommu_sva_get_pasid(struct iommu_sva *handle)
{
	return IOMMU_PASID_INVALID;
}
static inline void mm_pasid_init(struct mm_struct *mm) {}
static inline bool mm_valid_pasid(struct mm_struct *mm) { return false; }

static inline u32 mm_get_enqcmd_pasid(struct mm_struct *mm)
{
	return IOMMU_PASID_INVALID;
}

static inline void mm_pasid_drop(struct mm_struct *mm) {}
static inline void iommu_sva_invalidate_kva_range(unsigned long start, unsigned long end) {}

static inline int
iopf_queue_add_device(struct iopf_queue *queue, struct device *dev)
{
	return -ENODEV;
}

static inline void
iopf_queue_remove_device(struct iopf_queue *queue, struct device *dev)
{
}

static inline int iopf_queue_flush_dev(struct device *dev)
{
	return -ENODEV;
}

static inline struct iopf_queue *iopf_queue_alloc(const char *name)
{
	return NULL;
}

static inline void iopf_queue_free(struct iopf_queue *queue)
{
}

static inline int iopf_queue_discard_partial(struct iopf_queue *queue)
{
	return -ENODEV;
}

static inline void iopf_free_group(struct iopf_group *group)
{
}

static inline int
iommu_report_device_fault(struct device *dev, struct iopf_fault *evt)
{
	return -ENODEV;
}

static inline void iopf_group_response(struct iopf_group *group,
				       enum iommu_page_response_code status)
{
}
#endif /* __LINUX_IOMMU_H */
