# SPDX-License-Identifier: GPL-2.0
# Makefile for the Linux device tree

obj-y			:= component.o core.o bus.o dd.o syscore.o \
			   driver.o class.o platform.o \
			   cpu.o firmware.o init.o map.o devres.o \
			   attribute_container.o transport_class.o \
			   topology.o container.o property.o cacheinfo.o \
			   swnode.o faux.o
obj-$(CONFIG_AUXILIARY_BUS) += auxiliary.o
obj-$(CONFIG_DEVTMPFS)	+= devtmpfs.o
obj-y			+= power/
obj-y				+= firmware_loader/
obj-$(CONFIG_NUMA)	+= node.o
obj-$(CONFIG_MEMORY_HOTPLUG) += memory.o
ifeq ($(CONFIG_SYSFS),y)
obj-$(CONFIG_MODULES)	+= module.o
obj-$(CONFIG_AUXILIARY_BUS) += auxiliary_sysfs.o
endif
obj-$(CONFIG_REGMAP)	+= regmap/
obj-$(CONFIG_PINCTRL) += pinctrl.o
obj-$(CONFIG_GENERIC_MSI_IRQ) += platform-msi.o
obj-$(CONFIG_GENERIC_ARCH_TOPOLOGY) += arch_topology.o
obj-$(CONFIG_GENERIC_ARCH_NUMA) += arch_numa.o
obj-$(CONFIG_ACPI) += physical_location.o
