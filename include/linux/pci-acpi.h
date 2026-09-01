/* SPDX-License-Identifier: GPL-2.0 */
/*
 * File		pci-acpi.h
 *
 * Copyright (C) 2004 Intel
 * Copyright (C) Tom Long Nguyen (tom.l.nguyen@intel.com)
 */

#ifndef _PCI_ACPI_H_
#define _PCI_ACPI_H_

#include <linux/acpi.h>

#ifdef CONFIG_ACPI
extern phys_addr_t acpi_pci_root_get_mcfg_addr(acpi_handle handle);

static inline acpi_handle acpi_find_root_bridge_handle(struct pci_dev *pdev)
{
	struct pci_bus *pbus = pdev->bus;

	/* Find a PCI root bus */
	while (!pci_is_root_bus(pbus))
		pbus = pbus->parent;

	return ACPI_HANDLE(pbus->bridge);
}

static inline acpi_handle acpi_pci_get_bridge_handle(struct pci_bus *pbus)
{
	struct device *dev;

	if (pci_is_root_bus(pbus))
		dev = pbus->bridge;
	else {
		/* If pbus is a virtual bus, there is no bridge to it */
		if (!pbus->self)
			return NULL;

		dev = &pbus->self->dev;
	}

	return ACPI_HANDLE(dev);
}

struct acpi_pci_root;
struct acpi_pci_root_ops;

struct acpi_pci_root_info {
	struct acpi_pci_root		*root;
	struct acpi_device		*bridge;
	struct acpi_pci_root_ops	*ops;
	struct list_head		resources;
	char				name[16];
};

struct acpi_pci_root_ops {
	struct pci_ops *pci_ops;
	int (*init_info)(struct acpi_pci_root_info *info);
	void (*release_info)(struct acpi_pci_root_info *info);
	int (*prepare_resources)(struct acpi_pci_root_info *info);
};

extern int acpi_pci_probe_root_resources(struct acpi_pci_root_info *info);
extern struct pci_bus *acpi_pci_root_create(struct acpi_pci_root *root,
					    struct acpi_pci_root_ops *ops,
					    struct acpi_pci_root_info *info,
					    void *sd);

void acpi_pci_add_bus(struct pci_bus *bus);
void acpi_pci_remove_bus(struct pci_bus *bus);

#ifdef CONFIG_PCI
void pci_acpi_setup(struct device *dev, struct acpi_device *adev);
void pci_acpi_cleanup(struct device *dev, struct acpi_device *adev);
#else
static inline void pci_acpi_setup(struct device *dev, struct acpi_device *adev) {}
static inline void pci_acpi_cleanup(struct device *dev, struct acpi_device *adev) {}
#endif

static inline void acpi_pci_slot_init(void) { }
static inline void acpi_pci_slot_enumerate(struct pci_bus *bus) { }
static inline void acpi_pci_slot_remove(struct pci_bus *bus) { }

static inline void acpiphp_init(void) { }
static inline void acpiphp_enumerate_slots(struct pci_bus *bus) { }
static inline void acpiphp_remove_slots(struct pci_bus *bus) { }
static inline void acpiphp_check_host_bridge(struct acpi_device *adev) { }

extern const guid_t pci_acpi_dsm_guid;

/* _DSM Definitions for PCI */
#define DSM_PCI_PRESERVE_BOOT_CONFIG		0x05
#define DSM_PCI_DEVICE_NAME			0x07
#define DSM_PCI_POWER_ON_RESET_DELAY		0x08
#define DSM_PCI_DEVICE_READINESS_DURATIONS	0x09

static inline void pci_acpi_add_edr_notifier(struct pci_dev *pdev) { }
static inline void pci_acpi_remove_edr_notifier(struct pci_dev *pdev) { }

int pci_acpi_set_companion_lookup_hook(struct acpi_device *(*func)(struct pci_dev *));
void pci_acpi_clear_companion_lookup_hook(void);

#else	/* CONFIG_ACPI */
static inline void acpi_pci_add_bus(struct pci_bus *bus) { }
static inline void acpi_pci_remove_bus(struct pci_bus *bus) { }
#endif	/* CONFIG_ACPI */

#endif	/* _PCI_ACPI_H_ */
