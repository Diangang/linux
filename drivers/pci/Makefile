# SPDX-License-Identifier: GPL-2.0
#
# Makefile for the PCI bus specific drivers.

obj-$(CONFIG_PCI)		+= access.o bus.o probe.o host-bridge.o \
				   remove.o pci.o pci-driver.o search.o \
				   rebar.o rom.o setup-res.o irq.o vpd.o \
				   setup-bus.o vc.o mmap.o devres.o

obj-$(CONFIG_PCI)		+= msi/
ifdef CONFIG_PCI
obj-$(CONFIG_PROC_FS)		+= proc.o
obj-$(CONFIG_SYSFS)		+= pci-sysfs.o slot.o
obj-$(CONFIG_ACPI)		+= pci-acpi.o
obj-$(CONFIG_GENERIC_PCI_IOMAP) += iomap.o
endif

obj-$(CONFIG_OF)		+= of.o
obj-$(CONFIG_PCI_SYSCALL)	+= syscall.o
obj-$(CONFIG_PCI_ECAM)		+= ecam.o

obj-y				+= controller/
