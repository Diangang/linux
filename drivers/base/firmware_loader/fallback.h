/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __FIRMWARE_FALLBACK_H
#define __FIRMWARE_FALLBACK_H

#include <linux/firmware.h>
#include <linux/device.h>

#include "firmware.h"
#include "sysfs.h"

static inline int firmware_fallback_sysfs(struct firmware *fw, const char *name,
					  struct device *device,
					  u32 opt_flags,
					  int ret)
{
	/* Keep carrying over the same error */
	return ret;
}

static inline void kill_pending_fw_fallback_reqs(bool kill_all) { }
static inline void fw_fallback_set_cache_timeout(void) { }
static inline void fw_fallback_set_default_timeout(void) { }

static inline int firmware_fallback_platform(struct fw_priv *fw_priv)
{
	return -ENOENT;
}

#endif /* __FIRMWARE_FALLBACK_H */
