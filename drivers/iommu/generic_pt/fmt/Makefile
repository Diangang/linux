# SPDX-License-Identifier: GPL-2.0

iommu_pt_fmt-$(CONFIG_IOMMU_PT_AMDV1) += amdv1

iommu_pt_fmt-$(CONFIG_IOMMU_PT_VTDSS) += vtdss

iommu_pt_fmt-$(CONFIG_IOMMU_PT_X86_64) += x86_64

define create_format
obj-$(2) += iommu_$(1).o

endef

$(eval $(foreach fmt,$(iommu_pt_fmt-y),$(call create_format,$(fmt),y)))
$(eval $(foreach fmt,$(iommu_pt_fmt-m),$(call create_format,$(fmt),m)))
