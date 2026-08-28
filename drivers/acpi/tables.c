// SPDX-License-Identifier: GPL-2.0-or-later
/*
 *  acpi_tables.c - ACPI Boot-Time Table Parsing
 *
 *  Copyright (C) 2001 Paul Diefenbaugh <paul.s.diefenbaugh@intel.com>
 */

/* Uncomment next line to get verbose printout */
/* #define DEBUG */
#define pr_fmt(fmt) "ACPI: " fmt

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/smp.h>
#include <linux/string.h>
#include <linux/string_choices.h>
#include <linux/types.h>
#include <linux/irq.h>
#include <linux/errno.h>
#include <linux/acpi.h>
#include <linux/memblock.h>
#include <linux/earlycpio.h>
#include <linux/initrd.h>
#include <linux/security.h>
#include "internal.h"

#ifdef CONFIG_ACPI_CUSTOM_DSDT
#include CONFIG_ACPI_CUSTOM_DSDT_FILE
#endif

#define ACPI_MAX_TABLES		128

static char *mps_inti_flags_polarity[] = { "dfl", "high", "res", "low" };
static char *mps_inti_flags_trigger[] = { "dfl", "edge", "res", "level" };

static struct acpi_table_desc initial_tables[ACPI_MAX_TABLES] __initdata;

static int acpi_apic_instance __initdata_or_acpilib;

/*
 * Disable table checksum verification for the early stage due to the size
 * limitation of the current x86 early mapping implementation.
 */
static bool acpi_verify_table_checksum __initdata_or_acpilib = false;

void acpi_table_print_madt_entry(struct acpi_subtable_header *header)
{
	if (!header)
		return;

	switch (header->type) {

	case ACPI_MADT_TYPE_LOCAL_APIC:
		{
			struct acpi_madt_local_apic *p =
			    (struct acpi_madt_local_apic *)header;
			pr_debug("LAPIC (acpi_id[0x%02x] lapic_id[0x%02x] %s)\n",
				 p->processor_id, p->id,
				 str_enabled_disabled(p->lapic_flags & ACPI_MADT_ENABLED));
		}
		break;

	case ACPI_MADT_TYPE_LOCAL_X2APIC:
		{
			struct acpi_madt_local_x2apic *p =
			    (struct acpi_madt_local_x2apic *)header;
			pr_debug("X2APIC (apic_id[0x%02x] uid[0x%02x] %s)\n",
				 p->local_apic_id, p->uid,
				 str_enabled_disabled(p->lapic_flags & ACPI_MADT_ENABLED));
		}
		break;

	case ACPI_MADT_TYPE_IO_APIC:
		{
			struct acpi_madt_io_apic *p =
			    (struct acpi_madt_io_apic *)header;
			pr_debug("IOAPIC (id[0x%02x] address[0x%08x] gsi_base[%d])\n",
				 p->id, p->address, p->global_irq_base);
		}
		break;

	case ACPI_MADT_TYPE_INTERRUPT_OVERRIDE:
		{
			struct acpi_madt_interrupt_override *p =
			    (struct acpi_madt_interrupt_override *)header;
			pr_info("INT_SRC_OVR (bus %d bus_irq %d global_irq %d %s %s)\n",
				p->bus, p->source_irq, p->global_irq,
				mps_inti_flags_polarity[p->inti_flags & ACPI_MADT_POLARITY_MASK],
				mps_inti_flags_trigger[(p->inti_flags & ACPI_MADT_TRIGGER_MASK) >> 2]);
			if (p->inti_flags  &
			    ~(ACPI_MADT_POLARITY_MASK | ACPI_MADT_TRIGGER_MASK))
				pr_info("INT_SRC_OVR unexpected reserved flags: 0x%x\n",
					p->inti_flags  &
					~(ACPI_MADT_POLARITY_MASK | ACPI_MADT_TRIGGER_MASK));
		}
		break;

	case ACPI_MADT_TYPE_NMI_SOURCE:
		{
			struct acpi_madt_nmi_source *p =
			    (struct acpi_madt_nmi_source *)header;
			pr_info("NMI_SRC (%s %s global_irq %d)\n",
				mps_inti_flags_polarity[p->inti_flags & ACPI_MADT_POLARITY_MASK],
				mps_inti_flags_trigger[(p->inti_flags & ACPI_MADT_TRIGGER_MASK) >> 2],
				p->global_irq);
		}
		break;

	case ACPI_MADT_TYPE_LOCAL_APIC_NMI:
		{
			struct acpi_madt_local_apic_nmi *p =
			    (struct acpi_madt_local_apic_nmi *)header;
			pr_info("LAPIC_NMI (acpi_id[0x%02x] %s %s lint[0x%x])\n",
				p->processor_id,
				mps_inti_flags_polarity[p->inti_flags & ACPI_MADT_POLARITY_MASK	],
				mps_inti_flags_trigger[(p->inti_flags & ACPI_MADT_TRIGGER_MASK) >> 2],
				p->lint);
		}
		break;

	case ACPI_MADT_TYPE_LOCAL_X2APIC_NMI:
		{
			u16 polarity, trigger;
			struct acpi_madt_local_x2apic_nmi *p =
			    (struct acpi_madt_local_x2apic_nmi *)header;

			polarity = p->inti_flags & ACPI_MADT_POLARITY_MASK;
			trigger = (p->inti_flags & ACPI_MADT_TRIGGER_MASK) >> 2;

			pr_info("X2APIC_NMI (uid[0x%02x] %s %s lint[0x%x])\n",
				p->uid,
				mps_inti_flags_polarity[polarity],
				mps_inti_flags_trigger[trigger],
				p->lint);
		}
		break;

	case ACPI_MADT_TYPE_LOCAL_APIC_OVERRIDE:
		{
			struct acpi_madt_local_apic_override *p =
			    (struct acpi_madt_local_apic_override *)header;
			pr_info("LAPIC_ADDR_OVR (address[0x%llx])\n",
				p->address);
		}
		break;

	case ACPI_MADT_TYPE_IO_SAPIC:
		{
			struct acpi_madt_io_sapic *p =
			    (struct acpi_madt_io_sapic *)header;
			pr_debug("IOSAPIC (id[0x%x] address[%p] gsi_base[%d])\n",
				 p->id, (void *)(unsigned long)p->address,
				 p->global_irq_base);
		}
		break;

	case ACPI_MADT_TYPE_LOCAL_SAPIC:
		{
			struct acpi_madt_local_sapic *p =
			    (struct acpi_madt_local_sapic *)header;
			pr_debug("LSAPIC (acpi_id[0x%02x] lsapic_id[0x%02x] lsapic_eid[0x%02x] %s)\n",
				 p->processor_id, p->id, p->eid,
				 str_enabled_disabled(p->lapic_flags & ACPI_MADT_ENABLED));
		}
		break;

	case ACPI_MADT_TYPE_INTERRUPT_SOURCE:
		{
			struct acpi_madt_interrupt_source *p =
			    (struct acpi_madt_interrupt_source *)header;
			pr_info("PLAT_INT_SRC (%s %s type[0x%x] id[0x%04x] eid[0x%x] iosapic_vector[0x%x] global_irq[0x%x]\n",
				mps_inti_flags_polarity[p->inti_flags & ACPI_MADT_POLARITY_MASK],
				mps_inti_flags_trigger[(p->inti_flags & ACPI_MADT_TRIGGER_MASK) >> 2],
				p->type, p->id, p->eid, p->io_sapic_vector,
				p->global_irq);
		}
		break;

	case ACPI_MADT_TYPE_GENERIC_INTERRUPT:
		{
			struct acpi_madt_generic_interrupt *p =
				(struct acpi_madt_generic_interrupt *)header;
			pr_debug("GICC (acpi_id[0x%04x] address[%llx] MPIDR[0x%llx] %s)\n",
				 p->uid, p->base_address,
				 p->arm_mpidr,
				 str_enabled_disabled(p->flags & ACPI_MADT_ENABLED));

		}
		break;

	case ACPI_MADT_TYPE_GENERIC_DISTRIBUTOR:
		{
			struct acpi_madt_generic_distributor *p =
				(struct acpi_madt_generic_distributor *)header;
			pr_debug("GIC Distributor (gic_id[0x%04x] address[%llx] gsi_base[%d])\n",
				 p->gic_id, p->base_address,
				 p->global_irq_base);
		}
		break;

	case ACPI_MADT_TYPE_MULTIPROC_WAKEUP:
		{
			struct acpi_madt_multiproc_wakeup *p =
				(struct acpi_madt_multiproc_wakeup *)header;
			u64 reset_vector = 0;

			if (p->version >= ACPI_MADT_MP_WAKEUP_VERSION_V1)
				reset_vector = p->reset_vector;

			pr_debug("MP Wakeup (version[%d], mailbox[%#llx], reset[%#llx])\n",
				 p->version, p->mailbox_address, reset_vector);
		}
		break;

	case ACPI_MADT_TYPE_CORE_PIC:
		{
			struct acpi_madt_core_pic *p = (struct acpi_madt_core_pic *)header;

			pr_debug("CORE PIC (processor_id[0x%02x] core_id[0x%02x] %s)\n",
				 p->processor_id, p->core_id,
				 str_enabled_disabled(p->flags & ACPI_MADT_ENABLED));
		}
		break;

	case ACPI_MADT_TYPE_RINTC:
		{
			struct acpi_madt_rintc *p = (struct acpi_madt_rintc *)header;

			pr_debug("RISC-V INTC (acpi_uid[0x%04x] hart_id[0x%llx] %s)\n",
				 p->uid, p->hart_id,
				 str_enabled_disabled(p->flags & ACPI_MADT_ENABLED));
		}
		break;

	default:
		pr_warn("Found unsupported MADT entry (type = 0x%x)\n",
			header->type);
		break;
	}
}

int __init_or_acpilib acpi_table_parse_entries_array(
	char *id, unsigned long table_size, struct acpi_subtable_proc *proc,
	int proc_num, unsigned int max_entries)
{
	struct acpi_table_header *table_header = NULL;
	int count;
	u32 instance = 0;

	if (acpi_disabled)
		return -ENODEV;

	if (!id)
		return -EINVAL;

	if (!table_size)
		return -EINVAL;

	if (!strncmp(id, ACPI_SIG_MADT, 4))
		instance = acpi_apic_instance;

	acpi_get_table(id, instance, &table_header);
	if (!table_header) {
		pr_debug("%4.4s not present\n", id);
		return -ENODEV;
	}

	count = acpi_parse_entries_array(id, table_size,
					 (union fw_table_header *)table_header,
					 0, proc, proc_num, max_entries);

	acpi_put_table(table_header);
	return count;
}

static int __init_or_acpilib __acpi_table_parse_entries(
	char *id, unsigned long table_size, int entry_id,
	acpi_tbl_entry_handler handler, acpi_tbl_entry_handler_arg handler_arg,
	void *arg, unsigned int max_entries)
{
	struct acpi_subtable_proc proc = {
		.id		= entry_id,
		.handler	= handler,
		.handler_arg	= handler_arg,
		.arg		= arg,
	};

	return acpi_table_parse_entries_array(id, table_size, &proc, 1,
						max_entries);
}

int __init_or_acpilib
acpi_table_parse_cedt(enum acpi_cedt_type id,
		      acpi_tbl_entry_handler_arg handler_arg, void *arg)
{
	return __acpi_table_parse_entries(ACPI_SIG_CEDT,
					  sizeof(struct acpi_table_cedt), id,
					  NULL, handler_arg, arg, 0);
}
EXPORT_SYMBOL_ACPI_LIB(acpi_table_parse_cedt);

int __init acpi_table_parse_entries(char *id, unsigned long table_size,
				    int entry_id,
				    acpi_tbl_entry_handler handler,
				    unsigned int max_entries)
{
	return __acpi_table_parse_entries(id, table_size, entry_id, handler,
					  NULL, NULL, max_entries);
}

int __init acpi_table_parse_madt(enum acpi_madt_type id,
		      acpi_tbl_entry_handler handler, unsigned int max_entries)
{
	return acpi_table_parse_entries(ACPI_SIG_MADT,
					    sizeof(struct acpi_table_madt), id,
					    handler, max_entries);
}

/**
 * acpi_table_parse - find table with @id, run @handler on it
 * @id: table id to find
 * @handler: handler to run
 *
 * Scan the ACPI System Descriptor Table (STD) for a table matching @id,
 * run @handler on it.
 *
 * Return 0 if table found, -errno if not.
 */
int __init acpi_table_parse(char *id, acpi_tbl_table_handler handler)
{
	struct acpi_table_header *table = NULL;

	if (acpi_disabled)
		return -ENODEV;

	if (!id || !handler)
		return -EINVAL;

	if (strncmp(id, ACPI_SIG_MADT, 4) == 0)
		acpi_get_table(id, acpi_apic_instance, &table);
	else
		acpi_get_table(id, 0, &table);

	if (table) {
		handler(table);
		acpi_put_table(table);
		return 0;
	} else
		return -ENODEV;
}

/*
 * The BIOS is supposed to supply a single APIC/MADT,
 * but some report two.  Provide a knob to use either.
 * (don't you wish instance 0 and 1 were not the same?)
 */
static void __init check_multiple_madt(void)
{
	struct acpi_table_header *table = NULL;

	acpi_get_table(ACPI_SIG_MADT, 2, &table);
	if (table) {
		pr_warn("BIOS bug: multiple APIC/MADT found, using %d\n",
			acpi_apic_instance);
		pr_warn("If \"acpi_apic_instance=%d\" works better, "
			"notify linux-acpi@vger.kernel.org\n",
			acpi_apic_instance ? 0 : 2);
		acpi_put_table(table);

	} else
		acpi_apic_instance = 0;

	return;
}

static void acpi_table_taint(struct acpi_table_header *table)
{
	pr_warn("Override [%4.4s-%8.8s], this is unsafe: tainting kernel\n",
		table->signature, table->oem_table_id);
	add_taint(TAINT_OVERRIDDEN_ACPI_TABLE, LOCKDEP_NOW_UNRELIABLE);
}

static acpi_status
acpi_table_initrd_override(struct acpi_table_header *existing_table,
			   acpi_physical_address *address,
			   u32 *table_length)
{
	*table_length = 0;
	*address = 0;
	return AE_OK;
}

static void __init acpi_table_initrd_scan(void)
{
}

acpi_status
acpi_os_physical_table_override(struct acpi_table_header *existing_table,
				acpi_physical_address *address,
				u32 *table_length)
{
	return acpi_table_initrd_override(existing_table, address,
					  table_length);
}

#ifdef CONFIG_ACPI_CUSTOM_DSDT
static void *amlcode __attribute__ ((weakref("AmlCode")));
static void *dsdt_amlcode __attribute__ ((weakref("dsdt_aml_code")));
#endif

acpi_status acpi_os_table_override(struct acpi_table_header *existing_table,
		       struct acpi_table_header **new_table)
{
	if (!existing_table || !new_table)
		return AE_BAD_PARAMETER;

	*new_table = NULL;

#ifdef CONFIG_ACPI_CUSTOM_DSDT
	if (!strncmp(existing_table->signature, "DSDT", 4)) {
		*new_table = (struct acpi_table_header *)&amlcode;
		if (!(*new_table))
			*new_table = (struct acpi_table_header *)&dsdt_amlcode;
	}
#endif
	if (*new_table != NULL)
		acpi_table_taint(existing_table);
	return AE_OK;
}

/*
 * acpi_locate_initial_tables()
 *
 * Get the RSDP, then find and checksum all the ACPI tables.
 *
 * result: initial_tables[] is initialized, and points to
 * a list of ACPI tables.
 */
int __init acpi_locate_initial_tables(void)
{
	acpi_status status;

	if (acpi_verify_table_checksum) {
		pr_info("Early table checksum verification enabled\n");
		acpi_gbl_enable_table_validation = TRUE;
	} else {
		pr_info("Early table checksum verification disabled\n");
		acpi_gbl_enable_table_validation = FALSE;
	}

	status = acpi_initialize_tables(initial_tables, ACPI_MAX_TABLES, 0);
	if (ACPI_FAILURE(status)) {
		const char *msg = acpi_format_exception(status);

		pr_warn("Failed to initialize tables, status=0x%x (%s)", status, msg);
		return -EINVAL;
	}

	return 0;
}

void __init acpi_reserve_initial_tables(void)
{
	int i;

	for (i = 0; i < ACPI_MAX_TABLES; i++) {
		struct acpi_table_desc *table_desc = &initial_tables[i];
		u64 start = table_desc->address;
		u64 size = table_desc->length;

		if (!start || !size)
			break;

		pr_info("Reserving %4s table memory at [mem 0x%llx-0x%llx]\n",
			table_desc->signature.ascii, start, start + size - 1);

		memblock_reserve(start, size);
	}
}

void __init acpi_table_init_complete(void)
{
	acpi_table_initrd_scan();
	check_multiple_madt();
}

int __init acpi_table_init(void)
{
	int ret;

	ret = acpi_locate_initial_tables();
	if (ret)
		return ret;

	acpi_table_init_complete();

	return 0;
}

static int __init acpi_parse_apic_instance(char *str)
{
	if (!str)
		return -EINVAL;

	if (kstrtoint(str, 0, &acpi_apic_instance))
		return -EINVAL;

	pr_notice("Shall use APIC/MADT table %d\n", acpi_apic_instance);

	return 0;
}
early_param("acpi_apic_instance", acpi_parse_apic_instance);

static int __init acpi_force_table_verification_setup(char *s)
{
	acpi_verify_table_checksum = true;

	return 0;
}
early_param("acpi_force_table_verification", acpi_force_table_verification_setup);

static int __init acpi_force_32bit_fadt_addr(char *s)
{
	pr_info("Forcing 32 Bit FADT addresses\n");
	acpi_gbl_use32_bit_fadt_addresses = TRUE;

	return 0;
}
early_param("acpi_force_32bit_fadt_addr", acpi_force_32bit_fadt_addr);
