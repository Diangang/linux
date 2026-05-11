# SPDX-License-Identifier: GPL-2.0
#
# Makefile for linux/drivers/scsi
#
# 30 May 2000, Christoph Hellwig <hch@infradead.org>
# Rewritten to use lists instead of if-statements.
#
# 20 Sep 2000, Torben Mathiasen <tmm@image.dk>
# Changed link order to reflect new scsi initialization.
#
# *!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!
# The link order must be, SCSI Core, SCSI HBA drivers, and
# lastly SCSI peripheral drivers (disk/tape/cdrom/etc.) to
# satisfy certain initialization assumptions in the SCSI layer.
# *!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!

obj-$(CONFIG_SCSI)		+= scsi_mod.o
obj-$(CONFIG_SCSI_COMMON)	+= scsi_common.o

obj-$(CONFIG_SCSI_VIRTIO)	+= virtio_scsi.o

obj-$(CONFIG_BLK_DEV_SD)	+= sd_mod.o

# This goes last, so that "real" scsi devices probe earlier
scsi_mod-y			+= scsi.o hosts.o scsi_ioctl.o \
				   scsicam.o scsi_error.o scsi_lib.o
scsi_mod-$(CONFIG_SCSI_DMA)	+= scsi_lib_dma.o
scsi_mod-y			+= scsi_scan.o scsi_sysfs.o scsi_devinfo.o
scsi_mod-$(CONFIG_SYSCTL)	+= scsi_sysctl.o
scsi_mod-y			+= scsi_logging.o
scsi_mod-$(CONFIG_PM)		+= scsi_pm.o

sd_mod-objs	:= sd.o

$(obj)/scsi_sysfs.o: $(obj)/scsi_devinfo_tbl.c

quiet_cmd_bflags = GEN     $@
	cmd_bflags = sed -n 's/.*define *BLIST_\([A-Z0-9_]*\) *.*/BLIST_FLAG_NAME(\1),/p' $< > $@

$(obj)/scsi_devinfo_tbl.c: include/scsi/scsi_devinfo.h FORCE
	$(call if_changed,bflags)

targets +=  scsi_devinfo_tbl.c
