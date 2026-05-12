# SPDX-License-Identifier: GPL-2.0
obj-$(CONFIG_PCIE_DW) += pcie-designware.o
obj-$(CONFIG_PCIE_DW_HOST) += pcie-designware-host.o
# ARM32 platforms use hook_fault_code() and cannot support loadable module.
# On non-ARM32 platforms, loadable module can be supported.
obj-$(CONFIG_PCIE_KIRIN) += pcie-kirin.o

# The following drivers are for devices that use the generic ACPI
# pci_root.c driver but don't support standard ECAM config access.
# They contain MCFG quirks to replace the generic ECAM accessors with
# device-specific ones that are shared with the DT driver.

# The ACPI driver is generic and should not require driver-specific
# config options to be enabled, so we always build these drivers on
# ARM64 and use internal ifdefs to only build the pieces we need
# depending on whether ACPI, the DT driver, or both are enabled.

obj-$(CONFIG_PCI_HISI) += pcie-hisi.o

ifdef CONFIG_ACPI
ifdef CONFIG_PCI_QUIRKS
obj-$(CONFIG_ARM64) += pcie-al.o
obj-$(CONFIG_ARM64) += pcie-hisi.o
obj-$(CONFIG_ARM64) += pcie-tegra194-acpi.o
endif
endif
