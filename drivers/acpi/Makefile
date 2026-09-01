# SPDX-License-Identifier: GPL-2.0
#
# Makefile for the Linux ACPI interpreter
#

#
# ACPI Boot-Time Table Parsing
#
obj-$(CONFIG_ACPI)		+= tables.o

#
# ACPI Core Subsystem (Interpreter)
#
obj-$(CONFIG_ACPI)		+= acpi.o \
					acpica/

# All the builtin files are in the "acpi." module_param namespace.
acpi-y				+= osi.o osl.o utils.o reboot.o
acpi-y				+= nvs.o

acpi-y				+= device_sysfs.o device_pm.o


#
# ACPI Bus and Device Drivers
#
acpi-y				+= bus.o glue.o
acpi-y				+= scan.o
acpi-y				+= resource.o
acpi-$(CONFIG_PCI)		+= pci_root.o pci_link.o pci_irq.o
acpi-y				+= power.o
acpi-y				+= sysfs.o
acpi-y				+= property.o
