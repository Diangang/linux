# SPDX-License-Identifier: GPL-2.0
obj-y				:= i386.o init.o

obj-$(CONFIG_PCI_MMCONFIG)	+= mmconfig_$(BITS).o direct.o mmconfig-shared.o
obj-$(CONFIG_PCI_DIRECT)	+= direct.o


obj-y				+= fixup.o
obj-$(CONFIG_ACPI)		+= acpi.o
obj-y				+= legacy.o irq.o



obj-y				+= common.o early.o
obj-y				+= bus_numa.o

obj-$(CONFIG_AMD_NB)		+= amd_bus.o

