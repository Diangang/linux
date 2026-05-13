// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * CPU Microcode Update Driver for Linux
 *
 * Copyright (C) 2000-2006 Tigran Aivazian <aivazian.tigran@gmail.com>
 *	      2006	Shaohua Li <shaohua.li@intel.com>
 *	      2013-2016	Borislav Petkov <bp@alien8.de>
 *
 * X86 CPU microcode early update for Linux:
 *
 *	Copyright (C) 2012 Fenghua Yu <fenghua.yu@intel.com>
 *			   H Peter Anvin" <hpa@zytor.com>
 *		  (C) 2015 Borislav Petkov <bp@alien8.de>
 *
 * This driver allows to upgrade microcode on x86 processors.
 */

#define pr_fmt(fmt) "microcode: " fmt

#include <linux/stop_machine.h>
#include <linux/device/faux.h>
#include <linux/syscore_ops.h>
#include <linux/miscdevice.h>
#include <linux/capability.h>
#include <linux/firmware.h>
#include <linux/cpumask.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/cpu.h>
#include <linux/nmi.h>
#include <linux/fs.h>
#include <linux/mm.h>

#include <asm/apic.h>
#include <asm/cpu_device_id.h>
#include <asm/perf_event.h>
#include <asm/processor.h>
#include <asm/cmdline.h>
#include <asm/msr.h>
#include <asm/setup.h>

#include "internal.h"

static struct microcode_ops *microcode_ops;
static bool dis_ucode_ldr;

bool force_minrev;

/*
 * Those below should be behind CONFIG_MICROCODE_DBG ifdeffery but in
 * order to not uglify the code with ifdeffery and use IS_ENABLED()
 * instead, leave them in. When microcode debugging is not enabled,
 * those are meaningless anyway.
 */
/* base microcode revision for debugging */
u32 base_rev;
u32 microcode_rev[NR_CPUS] = {};

bool hypervisor_present;

/*
 * Synchronization.
 *
 * All non cpu-hotplug-callback call sites use:
 *
 * - cpus_read_lock/unlock() to synchronize with
 *   the cpu-hotplug-callback call sites.
 *
 * We guarantee that only a single cpu is being
 * updated at any particular moment of time.
 */
struct ucode_cpu_info		ucode_cpu_info[NR_CPUS];

/*
 * Those patch levels cannot be updated to newer ones and thus should be final.
 */
static u32 final_levels[] = {
	0x01000098,
	0x0100009f,
	0x010000af,
	0, /* T-101 terminator */
};

struct early_load_data early_data;

/*
 * Check the current patch level on this CPU.
 *
 * Returns:
 *  - true: if update should stop
 *  - false: otherwise
 */
static bool amd_check_current_patch_level(void)
{
	u32 lvl, dummy, i;
	u32 *levels;

	if (x86_cpuid_vendor() != X86_VENDOR_AMD)
		return false;

	native_rdmsr(MSR_AMD64_PATCH_LEVEL, lvl, dummy);

	levels = final_levels;

	for (i = 0; levels[i]; i++) {
		if (lvl == levels[i])
			return true;
	}
	return false;
}

bool __init microcode_loader_disabled(void)
{
	if (dis_ucode_ldr)
		return true;

	/*
	 * Disable when:
	 *
	 * 1) The CPU does not support CPUID.
	 */
	if (!cpuid_feature()) {
		dis_ucode_ldr = true;
		return dis_ucode_ldr;
	}

	/*
	 * 2) Bit 31 in CPUID[1]:ECX is clear
	 *    The bit is reserved for hypervisor use. This is still not
	 *    completely accurate as XEN PV guests don't see that CPUID bit
	 *    set, but that's good enough as they don't land on the BSP
	 *    path anyway.
	 *
	 * 3) Certain AMD patch levels are not allowed to be
	 *    overwritten.
	 */
	hypervisor_present = native_cpuid_ecx(1) & BIT(31);

	if ((hypervisor_present && !0) ||
	    amd_check_current_patch_level())
		dis_ucode_ldr = true;

	return dis_ucode_ldr;
}

static void __init early_parse_cmdline(void)
{
	char cmd_buf[64] = {};
	char *s, *p = cmd_buf;

	if (cmdline_find_option(boot_command_line, "microcode", cmd_buf, sizeof(cmd_buf)) > 0) {
		while ((s = strsep(&p, ","))) {
			if (0) {
				if (strstr(s, "base_rev=")) {
					/* advance to the option arg */
					strsep(&s, "=");
					if (kstrtouint(s, 16, &base_rev)) { ; }
				}
			}

			if (!strcmp("force_minrev", s))
				force_minrev = true;

			if (!strcmp(s, "dis_ucode_ldr"))
				dis_ucode_ldr = true;
		}
	}

	/* old, compat option */
	if (cmdline_find_option_bool(boot_command_line, "dis_ucode_ldr") > 0)
		dis_ucode_ldr = true;
}

void __init load_ucode_bsp(void)
{
	unsigned int cpuid_1_eax;
	bool intel = true;

	early_parse_cmdline();

	if (microcode_loader_disabled())
		return;

	cpuid_1_eax = native_cpuid_eax(1);

	switch (x86_cpuid_vendor()) {
	case X86_VENDOR_INTEL:
		if (x86_family(cpuid_1_eax) < 6)
			return;
		break;

	case X86_VENDOR_AMD:
		if (x86_family(cpuid_1_eax) < 0x10)
			return;
		intel = false;
		break;

	default:
		return;
	}

	if (intel)
		load_ucode_intel_bsp(&early_data);
	else
		load_ucode_amd_bsp(&early_data, cpuid_1_eax);
}

void load_ucode_ap(void)
{
	unsigned int cpuid_1_eax;

	/*
	 * Can't use microcode_loader_disabled() here - .init section
	 * hell. It doesn't have to either - the BSP variant must've
	 * parsed cmdline already anyway.
	 */
	if (dis_ucode_ldr)
		return;

	cpuid_1_eax = native_cpuid_eax(1);

	switch (x86_cpuid_vendor()) {
	case X86_VENDOR_INTEL:
		if (x86_family(cpuid_1_eax) >= 6)
			load_ucode_intel_ap();
		break;
	case X86_VENDOR_AMD:
		if (x86_family(cpuid_1_eax) >= 0x10)
			load_ucode_amd_ap(cpuid_1_eax);
		break;
	default:
		break;
	}
}

struct cpio_data __init find_microcode_in_initrd(const char *path)
{
#ifdef CONFIG_BLK_DEV_INITRD
	unsigned long start = 0;
	size_t size;

#ifdef CONFIG_X86_32
	size = boot_params.hdr.ramdisk_size;
	/* Early load on BSP has a temporary mapping. */
	if (size)
		start = initrd_start_early;

#else /* CONFIG_X86_64 */
	size  = (unsigned long)boot_params.ext_ramdisk_size << 32;
	size |= boot_params.hdr.ramdisk_size;

	if (size) {
		start  = (unsigned long)boot_params.ext_ramdisk_image << 32;
		start |= boot_params.hdr.ramdisk_image;
		start += PAGE_OFFSET;
	}
#endif

	/*
	 * Fixup the start address: after reserve_initrd() runs, initrd_start
	 * has the virtual address of the beginning of the initrd. It also
	 * possibly relocates the ramdisk. In either case, initrd_start contains
	 * the updated address so use that instead.
	 */
	if (initrd_start)
		start = initrd_start;

	return find_cpio_data(path, (void *)start, size, NULL);
#else /* !CONFIG_BLK_DEV_INITRD */
	return (struct cpio_data){ NULL, 0, "" };
#endif
}

static void reload_early_microcode(unsigned int cpu)
{
	int vendor, family;

	vendor = x86_cpuid_vendor();
	family = x86_cpuid_family();

	switch (vendor) {
	case X86_VENDOR_INTEL:
		if (family >= 6)
			reload_ucode_intel();
		break;
	case X86_VENDOR_AMD:
		if (family >= 0x10)
			reload_ucode_amd(cpu);
		break;
	default:
		break;
	}
}

/* fake device for request_firmware */
static struct faux_device *microcode_fdev;


static ssize_t version_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct ucode_cpu_info *uci = ucode_cpu_info + dev->id;

	return sprintf(buf, "0x%x\n", uci->cpu_sig.rev);
}

static ssize_t processor_flags_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct ucode_cpu_info *uci = ucode_cpu_info + dev->id;

	return sprintf(buf, "0x%x\n", uci->cpu_sig.pf);
}

static DEVICE_ATTR_RO(version);
static DEVICE_ATTR_RO(processor_flags);

static struct attribute *mc_default_attrs[] = {
	&dev_attr_version.attr,
	&dev_attr_processor_flags.attr,
	NULL
};

static const struct attribute_group mc_attr_group = {
	.attrs			= mc_default_attrs,
	.name			= "microcode",
};

static void microcode_fini_cpu(int cpu)
{
	if (microcode_ops->microcode_fini_cpu)
		microcode_ops->microcode_fini_cpu(cpu);
}

/**
 * microcode_bsp_resume - Update boot CPU microcode during resume.
 */
void microcode_bsp_resume(void)
{
	int cpu = smp_processor_id();
	struct ucode_cpu_info *uci = ucode_cpu_info + cpu;

	if (uci->mc)
		microcode_ops->apply_microcode(cpu);
	else
		reload_early_microcode(cpu);
}

static void microcode_bsp_syscore_resume(void *data)
{
	microcode_bsp_resume();
}

static const struct syscore_ops mc_syscore_ops = {
	.resume	= microcode_bsp_syscore_resume,
};

static struct syscore mc_syscore = {
	.ops = &mc_syscore_ops,
};

static int mc_cpu_online(unsigned int cpu)
{
	struct ucode_cpu_info *uci = ucode_cpu_info + cpu;
	struct device *dev = get_cpu_device(cpu);

	memset(uci, 0, sizeof(*uci));

	microcode_ops->collect_cpu_info(cpu, &uci->cpu_sig);
	cpu_data(cpu).microcode = uci->cpu_sig.rev;
	if (!cpu)
		boot_cpu_data.microcode = uci->cpu_sig.rev;

	if (sysfs_create_group(&dev->kobj, &mc_attr_group))
		pr_err("Failed to create group for CPU%d\n", cpu);
	return 0;
}

static int mc_cpu_down_prep(unsigned int cpu)
{
	struct device *dev = get_cpu_device(cpu);

	microcode_fini_cpu(cpu);
	sysfs_remove_group(&dev->kobj, &mc_attr_group);
	return 0;
}

static struct attribute *cpu_root_microcode_attrs[] = {
	NULL
};

static const struct attribute_group cpu_root_microcode_group = {
	.name  = "microcode",
	.attrs = cpu_root_microcode_attrs,
};

static int __init microcode_init(void)
{
	struct device *dev_root;
	struct cpuinfo_x86 *c = &boot_cpu_data;
	int error;

	if (microcode_loader_disabled())
		return -EINVAL;

	if (c->x86_vendor == X86_VENDOR_INTEL)
		microcode_ops = init_intel_microcode();
	else if (c->x86_vendor == X86_VENDOR_AMD)
		microcode_ops = init_amd_microcode();
	else
		pr_err("no support for this CPU vendor\n");

	if (!microcode_ops)
		return -ENODEV;

	pr_info_once("Current revision: 0x%08x\n", (early_data.new_rev ?: early_data.old_rev));

	if (early_data.new_rev)
		pr_info_once("Updated early from: 0x%08x\n", early_data.old_rev);

	microcode_fdev = faux_device_create("microcode", NULL, NULL);
	if (!microcode_fdev)
		return -ENODEV;

	dev_root = bus_get_dev_root(&cpu_subsys);
	if (dev_root) {
		error = sysfs_create_group(&dev_root->kobj, &cpu_root_microcode_group);
		put_device(dev_root);
		if (error) {
			pr_err("Error creating microcode group!\n");
			goto out_pdev;
		}
	}

	register_syscore(&mc_syscore);
	cpuhp_setup_state(CPUHP_AP_ONLINE_DYN, "x86/microcode:online",
			  mc_cpu_online, mc_cpu_down_prep);

	return 0;

 out_pdev:
	faux_device_destroy(microcode_fdev);
	return error;

}
late_initcall(microcode_init);
