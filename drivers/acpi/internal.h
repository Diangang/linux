/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * acpi/internal.h
 * For use by Linux/ACPI infrastructure, not drivers
 *
 * Copyright (c) 2009, Intel Corporation.
 */

#ifndef _ACPI_INTERNAL_H_
#define _ACPI_INTERNAL_H_

#include <linux/idr.h>

extern struct acpi_device *acpi_root;
extern struct list_head acpi_wakeup_device_list;
extern struct mutex acpi_device_lock;

int early_acpi_osi_init(void);
int acpi_osi_init(void);
acpi_status acpi_os_initialize1(void);
void acpi_scan_init(void);
#ifdef CONFIG_PCI
void acpi_pci_root_init(void);
void acpi_pci_link_init(void);
#else
static inline void acpi_pci_root_init(void) {}
static inline void acpi_pci_link_init(void) {}
#endif
int acpi_sysfs_init(void);
void acpi_gpe_apply_masked_gpes(void);
void acpi_sysfs_add_hotplug_profile(struct acpi_hotplug_profile *hotplug,
				    const char *name);
int acpi_scan_add_handler_with_hotplug(struct acpi_scan_handler *handler,
				       const char *hotplug_profile_name);
void acpi_scan_hotplug_enabled(struct acpi_hotplug_profile *hotplug, bool val);

static inline void acpi_debugfs_init(void) { return; }

acpi_status acpi_hotplug_schedule(struct acpi_device *adev, u32 src);
bool acpi_queue_hotplug_work(struct work_struct *work);
void acpi_device_hotplug(struct acpi_device *adev, u32 src);
bool acpi_scan_is_offline(struct acpi_device *adev, bool uevent);

acpi_status acpi_sysfs_table_handler(u32 event, void *table, void *context);
void acpi_scan_table_notify(void);

int acpi_active_trip_temp(struct acpi_device *adev, int id, int *ret_temp);
int acpi_passive_trip_temp(struct acpi_device *adev, int *ret_temp);
int acpi_hot_trip_temp(struct acpi_device *adev, int *ret_temp);
int acpi_critical_trip_temp(struct acpi_device *adev, int *ret_temp);

#ifdef CONFIG_ARM64
int acpi_arch_thermal_cpufreq_pctg(void);
#else
static inline int acpi_arch_thermal_cpufreq_pctg(void)
{
	return 0;
}
#endif

/* --------------------------------------------------------------------------
                     Device Node Initialization / Removal
   -------------------------------------------------------------------------- */
#define ACPI_STA_DEFAULT (ACPI_STA_DEVICE_PRESENT | ACPI_STA_DEVICE_ENABLED | \
			  ACPI_STA_DEVICE_UI | ACPI_STA_DEVICE_FUNCTIONING)

extern struct list_head acpi_bus_id_list;

struct acpi_device_bus_id {
	const char *bus_id;
	struct ida instance_ida;
	struct list_head node;
};

void acpi_init_device_object(struct acpi_device *device, acpi_handle handle,
			     int type, void (*release)(struct device *));
int acpi_tie_acpi_dev(struct acpi_device *adev);
int acpi_device_add(struct acpi_device *device);
void acpi_device_setup_files(struct acpi_device *dev);
void acpi_device_remove_files(struct acpi_device *dev);
extern const struct attribute_group *acpi_groups[];
void acpi_device_add_finalize(struct acpi_device *device);
void acpi_free_pnp_ids(struct acpi_device_pnp *pnp);
bool acpi_device_is_enabled(const struct acpi_device *adev);
bool acpi_device_is_present(const struct acpi_device *adev);
bool acpi_device_is_battery(struct acpi_device *adev);
bool acpi_device_is_first_physical_node(struct acpi_device *adev,
					const struct device *dev);

/* --------------------------------------------------------------------------
                     Device Matching and Notification
   -------------------------------------------------------------------------- */
const struct acpi_device *acpi_companion_match(const struct device *dev);
int __acpi_device_uevent_modalias(const struct acpi_device *adev,
				  struct kobj_uevent_env *env);

/* --------------------------------------------------------------------------
                                  Power Resource
   -------------------------------------------------------------------------- */
void acpi_power_resources_init(void);
void acpi_power_resources_list_free(struct list_head *list);
int acpi_extract_power_resources(union acpi_object *package, unsigned int start,
				 struct list_head *list);
struct acpi_device *acpi_add_power_resource(acpi_handle handle);
void acpi_power_add_remove_device(struct acpi_device *adev, bool add);
int acpi_power_wakeup_list_init(struct list_head *list, int *system_level);
int acpi_device_sleep_wake(struct acpi_device *dev,
			   int enable, int sleep_state, int dev_state);
int acpi_power_get_inferred_state(struct acpi_device *device, int *state);
int acpi_power_on_resources(struct acpi_device *device, int state);
int acpi_power_transition(struct acpi_device *device, int state);
void acpi_turn_off_unused_power_resources(void);

/* --------------------------------------------------------------------------
                              Device Power Management
   -------------------------------------------------------------------------- */
int acpi_device_get_power(struct acpi_device *device, int *state);

/*--------------------------------------------------------------------------
				Device properties
  -------------------------------------------------------------------------- */
#define ACPI_DT_NAMESPACE_HID	"PRP0001"

void acpi_init_properties(struct acpi_device *adev);
void acpi_free_properties(struct acpi_device *adev);

static inline bool acpi_graph_ignore_port(acpi_handle handle) { return false; }

#endif /* _ACPI_INTERNAL_H_ */
