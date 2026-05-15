// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * pnpacpi -- PnP ACPI driver
 *
 * Copyright (c) 2004 Matthieu Castet <castet.matthieu@free.fr>
 * Copyright (c) 2004 Li Shaohua <shaohua.li@intel.com>
 */

#include <linux/export.h>
#include <linux/acpi.h>
#include <linux/pnp.h>
#include <linux/slab.h>
#include <linux/mod_devicetable.h>

#include "../base.h"
#include "pnpacpi.h"

static int pnpacpi_get_resources(struct pnp_dev *dev)
{
	pnp_dbg(&dev->dev, "get resources\n");
	return pnpacpi_parse_allocated_resource(dev);
}

static int pnpacpi_set_resources(struct pnp_dev *dev)
{
	struct acpi_device *acpi_dev;
	acpi_handle handle;
	int ret = 0;

	pnp_dbg(&dev->dev, "set resources\n");

	acpi_dev = ACPI_COMPANION(&dev->dev);
	if (!acpi_dev) {
		dev_dbg(&dev->dev, "ACPI device not found in %s!\n", __func__);
		return -ENODEV;
	}

	if (WARN_ON_ONCE(acpi_dev != dev->data))
		dev->data = acpi_dev;

	handle = acpi_dev->handle;
	if (acpi_has_method(handle, METHOD_NAME__SRS)) {
		struct acpi_buffer buffer;

		ret = pnpacpi_build_resource_template(dev, &buffer);
		if (ret)
			return ret;

		ret = pnpacpi_encode_resources(dev, &buffer);
		if (!ret) {
			acpi_status status;

			status = acpi_set_current_resources(handle, &buffer);
			if (ACPI_FAILURE(status))
				ret = -EIO;
		}
		kfree(buffer.pointer);
	}
	if (!ret && acpi_device_power_manageable(acpi_dev))
		ret = acpi_device_set_power(acpi_dev, ACPI_STATE_D0);

	return ret;
}

static int pnpacpi_disable_resources(struct pnp_dev *dev)
{
	struct acpi_device *acpi_dev;
	acpi_status status;

	dev_dbg(&dev->dev, "disable resources\n");

	acpi_dev = ACPI_COMPANION(&dev->dev);
	if (!acpi_dev) {
		dev_dbg(&dev->dev, "ACPI device not found in %s!\n", __func__);
		return 0;
	}

	/* acpi_unregister_gsi(pnp_irq(dev, 0)); */
	if (acpi_device_power_manageable(acpi_dev))
		acpi_device_set_power(acpi_dev, ACPI_STATE_D3_COLD);

	/* continue even if acpi_device_set_power() fails */
	status = acpi_evaluate_object(acpi_dev->handle, "_DIS", NULL, NULL);
	if (ACPI_FAILURE(status) && status != AE_NOT_FOUND)
		return -ENODEV;

	return 0;
}

struct pnp_protocol pnpacpi_protocol = {
	.name	 = "Plug and Play ACPI",
	.get	 = pnpacpi_get_resources,
	.set	 = pnpacpi_set_resources,
	.disable = pnpacpi_disable_resources,
};
EXPORT_SYMBOL(pnpacpi_protocol);

static acpi_status __init pnpacpi_add_device_handler(acpi_handle handle,
						     u32 lvl, void *context,
						     void **rv)
{
	struct acpi_device *device = acpi_fetch_acpi_dev(handle);

	if (!device)
		return AE_CTRL_DEPTH;
	return AE_OK;
}

int pnpacpi_disabled __initdata;
static int __init pnpacpi_init(void)
{
	if (acpi_disabled || pnpacpi_disabled) {
		printk(KERN_INFO "pnp: PnP ACPI: disabled\n");
		return 0;
	}
	printk(KERN_INFO "pnp: PnP ACPI init\n");
	pnp_register_protocol(&pnpacpi_protocol);
	acpi_get_devices(NULL, pnpacpi_add_device_handler, NULL, NULL);
	printk(KERN_INFO "pnp: PnP ACPI: found 0 devices\n");
	pnp_platform_devices = 1;
	return 0;
}

fs_initcall(pnpacpi_init);

static int __init pnpacpi_setup(char *str)
{
	if (str == NULL)
		return 1;
	if (!strncmp(str, "off", 3))
		pnpacpi_disabled = 1;
	return 1;
}

__setup("pnpacpi=", pnpacpi_setup);
