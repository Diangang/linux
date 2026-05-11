# SPDX-License-Identifier: GPL-2.0
#
# Makefile for the Linux ACPI interpreter
#

ccflags-$(CONFIG_ACPI_DEBUG)	+= -DACPI_DEBUG_OUTPUT

ifdef CONFIG_TRACE_BRANCH_PROFILING
CFLAGS_processor_idle.o += -DDISABLE_BRANCH_PROFILING
endif

#
# ACPI Boot-Time Table Parsing
#
ifeq ($(CONFIG_ACPI_CUSTOM_DSDT),y)
tables.o: $(src)/../../include/$(CONFIG_ACPI_CUSTOM_DSDT_FILE) ;

endif

obj-$(CONFIG_ACPI)		+= tables.o

#
# ACPI Core Subsystem (Interpreter)
#
obj-$(CONFIG_ACPI)		+= acpi.o \
					acpica/

# All the builtin files are in the "acpi." module_param namespace.
acpi-y				+= osi.o osl.o utils.o reboot.o
acpi-y				+= nvs.o

# Power management related files
acpi-y				+= wakeup.o
acpi-$(CONFIG_ACPI_SYSTEM_POWER_STATES_SUPPORT) += sleep.o
acpi-y				+= device_sysfs.o device_pm.o
acpi-$(CONFIG_ACPI_SLEEP)	+= proc.o


#
# ACPI Bus and Device Drivers
#
acpi-y				+= bus.o glue.o
acpi-y				+= scan.o mipi-disco-img.o
acpi-y				+= resource.o
acpi-y				+= acpi_processor.o
acpi-y				+= processor_core.o
acpi-$(CONFIG_ARCH_MIGHT_HAVE_ACPI_PDC) += processor_pdc.o
acpi-$(CONFIG_ACPI_EC)		+= ec.o
acpi-$(CONFIG_ACPI_DOCK)	+= dock.o
acpi-$(CONFIG_PCI)		+= pci_root.o pci_link.o pci_irq.o
obj-$(CONFIG_ACPI_MCFG)		+= pci_mcfg.o
acpi-y				+= acpi_apd.o
acpi-y				+= acpi_platform.o
acpi-y				+= acpi_pnp.o
acpi-y				+= power.o
acpi-y				+= event.o
acpi-y				+= evged.o
acpi-y				+= sysfs.o
acpi-y				+= property.o
acpi-y				+= acpi_lpat.o
acpi-$(CONFIG_ACPI_LPIT)	+= acpi_lpit.o
acpi-$(CONFIG_ACPI_GENERIC_GSI) += irq.o
acpi-$(CONFIG_ACPI_PRMT)	+= prmt.o
acpi-$(CONFIG_ACPI_MRRM)	+= acpi_mrrm.o

# These are (potentially) separate modules

obj-$(CONFIG_ACPI_PROCESSOR)	+= processor.o
obj-$(CONFIG_ACPI)		+= container.o
obj-$(CONFIG_ACPI_NUMA)		+= numa/
obj-$(CONFIG_ACPI)		+= acpi_memhotplug.o
obj-$(CONFIG_ACPI_HOTPLUG_IOAPIC) += ioapic.o
obj-$(CONFIG_ACPI_HED)		+= hed.o
obj-$(CONFIG_ACPI_SPCR_TABLE)	+= spcr.o
obj-$(CONFIG_ACPI_PPTT) 	+= pptt.o

# processor has its own "processor." module_param namespace
processor-y			:= processor_driver.o processor_thermal.o
processor-$(CONFIG_ACPI_PROCESSOR_IDLE) += processor_idle.o
processor-$(CONFIG_ACPI_CPU_FREQ_PSS)	+= processor_throttling.o
processor-$(CONFIG_CPU_FREQ)	+= processor_perflib.o

obj-$(CONFIG_ACPI_APEI)		+= apei/

obj-$(CONFIG_ARM64)		+= arm64/

obj-$(CONFIG_X86)		+= x86/
