/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __FIRMWARE_SYSFS_H
#define __FIRMWARE_SYSFS_H

#include <linux/device.h>

#include "firmware.h"

MODULE_IMPORT_NS("FIRMWARE_LOADER_PRIVATE");

static inline int register_sysfs_loader(void)
{
	return 0;
}

static inline void unregister_sysfs_loader(void)
{
}

struct fw_sysfs {
	bool nowait;
	struct device dev;
	struct fw_priv *fw_priv;
	struct firmware *fw;
	void *fw_upload_priv;
};
#define to_fw_sysfs(__dev)	container_of_const(__dev, struct fw_sysfs, dev)

void __fw_load_abort(struct fw_priv *fw_priv);

static inline void fw_load_abort(struct fw_sysfs *fw_sysfs)
{
	struct fw_priv *fw_priv = fw_sysfs->fw_priv;

	__fw_load_abort(fw_priv);
}

struct fw_sysfs *
fw_create_instance(struct firmware *firmware, const char *fw_name,
		   struct device *device, u32 opt_flags);

static inline int fw_upload_start(struct fw_sysfs *fw_sysfs)
{
	return 0;
}

static inline void fw_upload_free(struct fw_sysfs *fw_sysfs)
{
}

#endif /* __FIRMWARE_SYSFS_H */
