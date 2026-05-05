# SPDX-License-Identifier: GPL-2.0
#
# Makefile for the Linux kernel device drivers.
#
# 15 Sep 2000, Christoph Hellwig <hch@infradead.org>
# Rewritten to use lists instead of if-statements.
#

obj-y				+= irqchip/
obj-y				+= bus/


obj-y				+= pci/

obj-$(CONFIG_ACPI)		+= acpi/

# PnP must come after ACPI since it will eventually need to check if acpi
# was used and do nothing if so
obj-$(CONFIG_PNP)		+= pnp/
obj-y				+= amba/

obj-y				+= clk/
# Many drivers will want to use DMA so this has to be made available
# really early.
obj-$(CONFIG_DMADEVICES)	+= dma/

obj-y				+= virtio/

# tty/ comes before char/ so that the VT console is the boot-time
# default.
obj-y				+= tty/
obj-y				+= char/

obj-y				+= iommu/

obj-y				+= base/ block/
obj-y				+= scsi/
obj-y				+= nvme/
obj-y				+= firmware/
obj-y				+= clocksource/
obj-$(CONFIG_OF)		+= of/
