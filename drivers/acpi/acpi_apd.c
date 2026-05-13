// SPDX-License-Identifier: GPL-2.0-only
/*
 * AMD ACPI support for ACPI2platform device.
 *
 * Copyright (c) 2014,2015 AMD Corporation.
 * Authors: Ken Xue <Ken.Xue@amd.com>
 *	Wu, Jeff <Jeff.Wu@amd.com>
 */

#include <linux/acpi.h>
#include <linux/clkdev.h>
#include <linux/clk-provider.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/platform_data/clk-fch.h>
#include <linux/platform_device.h>

#include "internal.h"

struct apd_private_data;

/**
 * struct apd_device_desc - a descriptor for apd device
 * @fixed_clk_rate: fixed rate input clock source for acpi device;
 *			0 means no fixed rate input clock source
 * @properties: build-in properties of the device such as UART
 * @setup: a hook routine to set device resource during create platform device
 *
 * Device description defined as acpi_device_id.driver_data
 */
struct apd_device_desc {
	unsigned int fixed_clk_rate;
	struct property_entry *properties;
	int (*setup)(struct apd_private_data *pdata);
};

struct apd_private_data {
	struct clk *clk;
	struct acpi_device *adev;
	const struct apd_device_desc *dev_desc;
};

#if 0 || defined(CONFIG_ARM64)
#define APD_ADDR(desc)	((unsigned long)&desc)

static int acpi_apd_setup(struct apd_private_data *pdata)
{
	const struct apd_device_desc *dev_desc = pdata->dev_desc;
	struct clk *clk;

	if (dev_desc->fixed_clk_rate) {
		clk = clk_register_fixed_rate(&pdata->adev->dev,
					dev_name(&pdata->adev->dev),
					NULL, 0, dev_desc->fixed_clk_rate);
		clk_register_clkdev(clk, NULL, dev_name(&pdata->adev->dev));
		pdata->clk = clk;
	}

	return 0;
}


#ifdef CONFIG_ARM64
static const struct apd_device_desc xgene_i2c_desc = {
	.setup = acpi_apd_setup,
	.fixed_clk_rate = 100000000,
};

static const struct apd_device_desc vulcan_spi_desc = {
	.setup = acpi_apd_setup,
	.fixed_clk_rate = 133000000,
};

static const struct apd_device_desc hip07_i2c_desc = {
	.setup = acpi_apd_setup,
	.fixed_clk_rate = 200000000,
};

static const struct apd_device_desc hip08_i2c_desc = {
	.setup = acpi_apd_setup,
	.fixed_clk_rate = 250000000,
};

static const struct apd_device_desc hip08_lite_i2c_desc = {
	.setup = acpi_apd_setup,
	.fixed_clk_rate = 125000000,
};

static const struct apd_device_desc thunderx2_i2c_desc = {
	.setup = acpi_apd_setup,
	.fixed_clk_rate = 125000000,
};

static const struct apd_device_desc nxp_i2c_desc = {
	.setup = acpi_apd_setup,
	.fixed_clk_rate = 350000000,
};

static const struct apd_device_desc hip08_spi_desc = {
	.setup = acpi_apd_setup,
	.fixed_clk_rate = 250000000,
};
#endif /* CONFIG_ARM64 */

#endif

/*
 * Create platform device during acpi scan attach handle.
 * Return value > 0 on success of creating device.
 */
static int acpi_apd_create_device(struct acpi_device *adev,
				   const struct acpi_device_id *id)
{
	const struct apd_device_desc *dev_desc = (void *)id->driver_data;
	struct apd_private_data *pdata;
	struct platform_device *pdev;
	int ret;

	if (!dev_desc) {
		pdev = acpi_create_platform_device(adev, NULL);
		return IS_ERR_OR_NULL(pdev) ? PTR_ERR(pdev) : 1;
	}

	pdata = kzalloc_obj(*pdata);
	if (!pdata)
		return -ENOMEM;

	pdata->adev = adev;
	pdata->dev_desc = dev_desc;

	if (dev_desc->setup) {
		ret = dev_desc->setup(pdata);
		if (ret)
			goto err_out;
	}

	adev->driver_data = pdata;
	pdev = acpi_create_platform_device(adev, dev_desc->properties);
	if (!IS_ERR_OR_NULL(pdev))
		return 1;

	ret = PTR_ERR(pdev);
	adev->driver_data = NULL;

 err_out:
	kfree(pdata);
	return ret;
}

static const struct acpi_device_id acpi_apd_device_ids[] = {
	/* Generic apd devices */
#ifdef CONFIG_ARM64
	{ "APMC0D0F", APD_ADDR(xgene_i2c_desc) },
	{ "BRCM900D", APD_ADDR(vulcan_spi_desc) },
	{ "CAV900D",  APD_ADDR(vulcan_spi_desc) },
	{ "CAV9007",  APD_ADDR(thunderx2_i2c_desc) },
	{ "HISI02A1", APD_ADDR(hip07_i2c_desc) },
	{ "HISI02A2", APD_ADDR(hip08_i2c_desc) },
	{ "HISI02A3", APD_ADDR(hip08_lite_i2c_desc) },
	{ "HISI0173", APD_ADDR(hip08_spi_desc) },
	{ "NXP0001", APD_ADDR(nxp_i2c_desc) },
#endif
	{ }
};

static struct acpi_scan_handler apd_handler = {
	.ids = acpi_apd_device_ids,
	.attach = acpi_apd_create_device,
};

void __init acpi_apd_init(void)
{
	acpi_scan_add_handler(&apd_handler);
}
