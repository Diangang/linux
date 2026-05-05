# SPDX-License-Identifier: GPL-2.0
# dmaengine debug flags
subdir-ccflags-$(CONFIG_DMADEVICES_VDEBUG) += -DVERBOSE_DEBUG

# core
obj-$(CONFIG_DMA_ENGINE) += dmaengine.o
obj-$(CONFIG_DMA_VIRTUAL_CHANNELS) += virt-dma.o
obj-$(CONFIG_DMA_ACPI) += acpi-dma.o
obj-$(CONFIG_DMA_OF) += of-dma.o

# devices
obj-$(CONFIG_DW_DMAC_CORE) += dw/
fsl-edma-objs := fsl-edma-main.o fsl-edma-common.o
obj-$(CONFIG_FSL_EDMA) += fsl-edma.o
obj-$(CONFIG_HSU_DMA) += hsu/
obj-$(CONFIG_MV_XOR_V2) += mv_xor_v2.o
obj-$(CONFIG_PL330_DMA) += pl330.o
obj-y += qcom/
