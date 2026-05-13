# SPDX-License-Identifier: GPL-2.0
obj-$(CONFIG_CPU_SUP_AMD)		+= core.o lbr.o
obj-$(CONFIG_X86_LOCAL_APIC)		+= ibs.o
ifdef CONFIG_AMD_IOMMU
obj-$(CONFIG_CPU_SUP_AMD)		+= iommu.o
endif
