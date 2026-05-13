// SPDX-License-Identifier: GPL-2.0-or-later
/*
 *  blacklist.c
 *
 *  Check to see if the given machine has a known bad ACPI BIOS
 *  or if the BIOS is too old.
 *
 *  Copyright (C) 2004 Len Brown <len.brown@intel.com>
 *  Copyright (C) 2002 Andy Grover <andrew.grover@intel.com>
 */

#define pr_fmt(fmt) "ACPI: " fmt

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/acpi.h>

#include "../internal.h"

/*
 * POLICY: If *anything* doesn't work, put it on the blacklist.
 *	   If they are critical errors, mark it critical, and abort driver load.
 */
static struct acpi_platform_list acpi_blacklist[] __initdata = {
	/* Compaq Presario 1700 */
	{"PTLTD ", "  DSDT  ", 0x06040000, ACPI_SIG_DSDT, less_than_or_equal,
	 "Multiple problems", 1},
	/* Sony FX120, FX140, FX150? */
	{"SONY  ", "U0      ", 0x20010313, ACPI_SIG_DSDT, less_than_or_equal,
	 "ACPI driver problem", 1},
	/* Compaq Presario 800, Insyde BIOS */
	{"INT440", "SYSFexxx", 0x00001001, ACPI_SIG_DSDT, less_than_or_equal,
	 "Does not use _REG to protect EC OpRegions", 1},
	/* IBM 600E - _ADR should return 7, but it returns 1 */
	{"IBM   ", "TP600E  ", 0x00000105, ACPI_SIG_DSDT, less_than_or_equal,
	 "Incorrect _ADR", 1},

	{ }
};

int __init acpi_blacklisted(void)
{
	int i;
	int blacklisted = 0;

	i = acpi_match_platform_list(acpi_blacklist);
	if (i >= 0) {
		pr_err("Vendor \"%6.6s\" System \"%8.8s\" Revision 0x%x has a known ACPI BIOS problem.\n",
		       acpi_blacklist[i].oem_id,
		       acpi_blacklist[i].oem_table_id,
		       acpi_blacklist[i].oem_revision);

		pr_err("Reason: %s. This is a %s error\n",
		       acpi_blacklist[i].reason,
		       (acpi_blacklist[i].data ?
			"non-recoverable" : "recoverable"));

		blacklisted = acpi_blacklist[i].data;
	}

	(void)early_acpi_osi_init();

	return blacklisted;
}
