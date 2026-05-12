# SPDX-License-Identifier: GPL-2.0
obj-$(CONFIG_PCI_HOST_COMMON) += pci-host-common.o
obj-$(CONFIG_PCI_HOST_GENERIC) += pci-host-generic.o
obj-$(CONFIG_PCI_HOST_THUNDER_ECAM) += pci-thunder-ecam.o
obj-$(CONFIG_PCI_HOST_THUNDER_PEM) += pci-thunder-pem.o
obj-$(CONFIG_PCIE_XILINX) += pcie-xilinx.o
obj-$(CONFIG_PCI_XGENE) += pci-xgene.o
obj-$(CONFIG_PCI_XGENE_MSI) += pci-xgene-msi.o
obj-$(CONFIG_VMD) += vmd.o

# pcie-hisi.o quirks are needed even without CONFIG_PCIE_DW
obj-y				+= dwc/
obj-y				+= mobiveil/
obj-y				+= plda/

# The following drivers are for devices that use the generic ACPI
# pci_root.c driver but don't support standard ECAM config access.
# They contain MCFG quirks to replace the generic ECAM accessors with
# device-specific ones that are shared with the DT driver.

# The ACPI driver is generic and should not require driver-specific
# config options to be enabled, so we always build these drivers on
# ARM64 and use internal ifdefs to only build the pieces we need
# depending on whether ACPI, the DT driver, or both are enabled.

ifdef CONFIG_ACPI
ifdef CONFIG_PCI_QUIRKS
obj-$(CONFIG_ARM64) += pci-thunder-ecam.o
obj-$(CONFIG_ARM64) += pci-thunder-pem.o
obj-$(CONFIG_ARM64) += pci-xgene.o
endif
endif
