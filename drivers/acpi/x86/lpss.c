// SPDX-License-Identifier: GPL-2.0-only
/*
 * ACPI support for Intel Lynxpoint LPSS.
 *
 * Copyright (C) 2013, Intel Corporation
 * Authors: Mika Westerberg <mika.westerberg@linux.intel.com>
 *          Rafael J. Wysocki <rafael.j.wysocki@intel.com>
 */

#include <linux/acpi.h>
#include <linux/clkdev.h>
#include <linux/clk-provider.h>
#include <linux/dmi.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/mutex.h>
#include <linux/pci.h>
#include <linux/platform_device.h>
#include <linux/platform_data/x86/clk-lpss.h>
#include <linux/platform_data/x86/pmc_atom.h>
#include <linux/pm_domain.h>
#include <linux/pm_runtime.h>
#include <linux/pwm.h>
#include <linux/pxa2xx_ssp.h>
#include <linux/suspend.h>
#include <linux/delay.h>

#include "../internal.h"


#define LPSS_ADDR(desc) (0UL)


static const struct acpi_device_id acpi_lpss_device_ids[] = {
	/* Generic LPSS devices */
	{ "INTL9C60", LPSS_ADDR(lpss_dma_desc) },

	/* Lynxpoint LPSS devices */
	{ "INT33C0", LPSS_ADDR(lpt_spi_dev_desc) },
	{ "INT33C1", LPSS_ADDR(lpt_spi_dev_desc) },
	{ "INT33C2", LPSS_ADDR(lpt_i2c_dev_desc) },
	{ "INT33C3", LPSS_ADDR(lpt_i2c_dev_desc) },
	{ "INT33C4", LPSS_ADDR(lpt_uart_dev_desc) },
	{ "INT33C5", LPSS_ADDR(lpt_uart_dev_desc) },
	{ "INT33C6", LPSS_ADDR(lpt_sdio_dev_desc) },

	/* BayTrail LPSS devices */
	{ "80860F09", LPSS_ADDR(byt_pwm_dev_desc) },
	{ "80860F0A", LPSS_ADDR(byt_uart_dev_desc) },
	{ "80860F0E", LPSS_ADDR(byt_spi_dev_desc) },
	{ "80860F14", LPSS_ADDR(byt_sdio_dev_desc) },
	{ "80860F41", LPSS_ADDR(byt_i2c_dev_desc) },

	/* Braswell LPSS devices */
	{ "80862286", LPSS_ADDR(lpss_dma_desc) },
	{ "80862288", LPSS_ADDR(bsw_pwm_dev_desc) },
	{ "80862289", LPSS_ADDR(bsw_pwm2_dev_desc) },
	{ "8086228A", LPSS_ADDR(bsw_uart_dev_desc) },
	{ "8086228E", LPSS_ADDR(bsw_spi_dev_desc) },
	{ "808622C0", LPSS_ADDR(lpss_dma_desc) },
	{ "808622C1", LPSS_ADDR(bsw_i2c_dev_desc) },

	/* Broadwell LPSS devices */
	{ "INT3430", LPSS_ADDR(lpt_spi_dev_desc) },
	{ "INT3431", LPSS_ADDR(lpt_spi_dev_desc) },
	{ "INT3432", LPSS_ADDR(lpt_i2c_dev_desc) },
	{ "INT3433", LPSS_ADDR(lpt_i2c_dev_desc) },
	{ "INT3434", LPSS_ADDR(lpt_uart_dev_desc) },
	{ "INT3435", LPSS_ADDR(lpt_uart_dev_desc) },
	{ "INT3436", LPSS_ADDR(lpt_sdio_dev_desc) },

	{ }
};


static struct acpi_scan_handler lpss_handler = {
	.ids = acpi_lpss_device_ids,
};

void __init acpi_lpss_init(void)
{
	acpi_scan_add_handler(&lpss_handler);
}
