// SPDX-License-Identifier: LGPL-2.0+
/*
 * MTRR (Memory Type Range Register) cleanup
 *
 *  Copyright (C) 2009 Yinghai Lu
 */
#include <linux/init.h>
#include <linux/pci.h>
#include <linux/smp.h>
#include <linux/cpu.h>
#include <linux/mutex.h>
#include <linux/uaccess.h>
#include <linux/kvm_para.h>
#include <linux/range.h>

#include <asm/processor.h>
#include <asm/e820/api.h>
#include <asm/mtrr.h>
#include <asm/msr.h>

#include "mtrr.h"

struct var_mtrr_range_state {
	unsigned long	base_pfn;
	unsigned long	size_pfn;
	mtrr_type	type;
};

struct var_mtrr_state {
	unsigned long	range_startk;
	unsigned long	range_sizek;
	unsigned long	chunk_sizek;
	unsigned long	gran_sizek;
	unsigned int	reg;
};

/* Should be related to MTRR_VAR_RANGES nums */
#define RANGE_NUM				256

static struct range __initdata		range[RANGE_NUM];
static int __initdata				nr_range;

static struct var_mtrr_range_state __initdata	range_state[RANGE_NUM];

#define BIOS_BUG_MSG \
	"WARNING: BIOS bug: VAR MTRR %d contains strange UC entry under 1M, check with your system vendor!\n"

static int __init
x86_get_mtrr_mem_range(struct range *range, int nr_range,
		       unsigned long extra_remove_base,
		       unsigned long extra_remove_size)
{
	unsigned long base, size;
	mtrr_type type;
	int i;

	for (i = 0; i < num_var_ranges; i++) {
		type = range_state[i].type;
		if (type != MTRR_TYPE_WRBACK)
			continue;
		base = range_state[i].base_pfn;
		size = range_state[i].size_pfn;
		nr_range = add_range_with_merge(range, RANGE_NUM, nr_range,
						base, base + size);
	}

	Dprintk("After WB checking\n");
	for (i = 0; i < nr_range; i++)
		Dprintk("MTRR MAP PFN: %016llx - %016llx\n",
			 range[i].start, range[i].end);

	/* Take out UC ranges: */
	for (i = 0; i < num_var_ranges; i++) {
		type = range_state[i].type;
		if (type != MTRR_TYPE_UNCACHABLE &&
		    type != MTRR_TYPE_WRPROT)
			continue;
		size = range_state[i].size_pfn;
		if (!size)
			continue;
		base = range_state[i].base_pfn;
		if (base < (1<<(20-PAGE_SHIFT)) && mtrr_state.have_fixed &&
		    (mtrr_state.enabled & MTRR_STATE_MTRR_ENABLED) &&
		    (mtrr_state.enabled & MTRR_STATE_MTRR_FIXED_ENABLED)) {
			/* Var MTRR contains UC entry below 1M? Skip it: */
			pr_warn(BIOS_BUG_MSG, i);
			if (base + size <= (1<<(20-PAGE_SHIFT)))
				continue;
			size -= (1<<(20-PAGE_SHIFT)) - base;
			base = 1<<(20-PAGE_SHIFT);
		}
		subtract_range(range, RANGE_NUM, base, base + size);
	}
	if (extra_remove_size)
		subtract_range(range, RANGE_NUM, extra_remove_base,
				 extra_remove_base + extra_remove_size);

	Dprintk("After UC checking\n");
	for (i = 0; i < RANGE_NUM; i++) {
		if (!range[i].end)
			continue;

		Dprintk("MTRR MAP PFN: %016llx - %016llx\n",
			 range[i].start, range[i].end);
	}

	/* sort the ranges */
	nr_range = clean_sort_range(range, RANGE_NUM);

	Dprintk("After sorting\n");
	for (i = 0; i < nr_range; i++)
		Dprintk("MTRR MAP PFN: %016llx - %016llx\n",
			range[i].start, range[i].end);

	return nr_range;
}

int __init mtrr_cleanup(void)
{
	return 0;
}

static int disable_mtrr_trim;

static int __init disable_mtrr_trim_setup(char *str)
{
	disable_mtrr_trim = 1;
	return 0;
}
early_param("disable_mtrr_trim", disable_mtrr_trim_setup);

/*
 * Newer AMD K8s and later CPUs have a special magic MSR way to force WB
 * for memory >4GB. Check for that here.
 * Note this won't check if the MTRRs < 4GB where the magic bit doesn't
 * apply to are wrong, but so far we don't know of any such case in the wild.
 */
#define Tom2Enabled		(1U << 21)
#define Tom2ForceMemTypeWB	(1U << 22)

int __init amd_special_default_mtrr(void)
{
	u32 l, h;

	if (boot_cpu_data.x86_vendor != X86_VENDOR_AMD &&
	    boot_cpu_data.x86_vendor != X86_VENDOR_HYGON)
		return 0;
	if (boot_cpu_data.x86 < 0xf)
		return 0;
	/* In case some hypervisor doesn't pass SYSCFG through: */
	if (rdmsr_safe(MSR_AMD64_SYSCFG, &l, &h) < 0)
		return 0;
	/*
	 * Memory between 4GB and top of mem is forced WB by this magic bit.
	 * Reserved before K8RevF, but should be zero there.
	 */
	if ((l & (Tom2Enabled | Tom2ForceMemTypeWB)) ==
		 (Tom2Enabled | Tom2ForceMemTypeWB))
		return 1;
	return 0;
}

static u64 __init
real_trim_memory(unsigned long start_pfn, unsigned long limit_pfn)
{
	u64 trim_start, trim_size;

	trim_start = start_pfn;
	trim_start <<= PAGE_SHIFT;

	trim_size = limit_pfn;
	trim_size <<= PAGE_SHIFT;
	trim_size -= trim_start;

	return e820__range_update(trim_start, trim_size, E820_TYPE_RAM, E820_TYPE_RESERVED);
}

/**
 * mtrr_trim_uncached_memory - trim RAM not covered by MTRRs
 * @end_pfn: ending page frame number
 *
 * Some buggy BIOSes don't setup the MTRRs properly for systems with certain
 * memory configurations.  This routine checks that the highest MTRR matches
 * the end of memory, to make sure the MTRRs having a write back type cover
 * all of the memory the kernel is intending to use.  If not, it'll trim any
 * memory off the end by adjusting end_pfn, removing it from the kernel's
 * allocation pools, warning the user with an obnoxious message.
 */
int __init mtrr_trim_uncached_memory(unsigned long end_pfn)
{
	unsigned long i, base, size, highest_pfn = 0, def, dummy;
	mtrr_type type;
	u64 total_trim_size;
	/* extra one for all 0 */
	int num[MTRR_NUM_TYPES + 1];

	if (!mtrr_enabled())
		return 0;

	/*
	 * Make sure we only trim uncachable memory on machines that
	 * support the Intel MTRR architecture:
	 */
	if (!cpu_feature_enabled(X86_FEATURE_MTRR) || disable_mtrr_trim)
		return 0;

	rdmsr(MSR_MTRRdefType, def, dummy);
	def &= MTRR_DEF_TYPE_TYPE;
	if (def != MTRR_TYPE_UNCACHABLE)
		return 0;

	/* Get it and store it aside: */
	memset(range_state, 0, sizeof(range_state));
	for (i = 0; i < num_var_ranges; i++) {
		mtrr_if->get(i, &base, &size, &type);
		range_state[i].base_pfn = base;
		range_state[i].size_pfn = size;
		range_state[i].type = type;
	}

	/* Find highest cached pfn: */
	for (i = 0; i < num_var_ranges; i++) {
		type = range_state[i].type;
		if (type != MTRR_TYPE_WRBACK)
			continue;
		base = range_state[i].base_pfn;
		size = range_state[i].size_pfn;
		if (highest_pfn < base + size)
			highest_pfn = base + size;
	}

	/* kvm/qemu doesn't have mtrr set right, don't trim them all: */
	if (!highest_pfn) {
		pr_info("CPU MTRRs all blank - virtualized system.\n");
		return 0;
	}

	/* Check entries number: */
	memset(num, 0, sizeof(num));
	for (i = 0; i < num_var_ranges; i++) {
		type = range_state[i].type;
		if (type >= MTRR_NUM_TYPES)
			continue;
		size = range_state[i].size_pfn;
		if (!size)
			type = MTRR_NUM_TYPES;
		num[type]++;
	}

	/* No entry for WB? */
	if (!num[MTRR_TYPE_WRBACK])
		return 0;

	/* Check if we only had WB and UC: */
	if (num[MTRR_TYPE_WRBACK] + num[MTRR_TYPE_UNCACHABLE] !=
		num_var_ranges - num[MTRR_NUM_TYPES])
		return 0;

	memset(range, 0, sizeof(range));
	nr_range = 0;
	if (mtrr_tom2) {
		range[nr_range].start = (1ULL<<(32 - PAGE_SHIFT));
		range[nr_range].end = mtrr_tom2 >> PAGE_SHIFT;
		if (highest_pfn < range[nr_range].end)
			highest_pfn = range[nr_range].end;
		nr_range++;
	}
	nr_range = x86_get_mtrr_mem_range(range, nr_range, 0, 0);

	/* Check the head: */
	total_trim_size = 0;
	if (range[0].start)
		total_trim_size += real_trim_memory(0, range[0].start);

	/* Check the holes: */
	for (i = 0; i < nr_range - 1; i++) {
		if (range[i].end < range[i+1].start)
			total_trim_size += real_trim_memory(range[i].end,
							    range[i+1].start);
	}

	/* Check the top: */
	i = nr_range - 1;
	if (range[i].end < end_pfn)
		total_trim_size += real_trim_memory(range[i].end,
							 end_pfn);

	if (total_trim_size) {
		pr_warn("WARNING: BIOS bug: CPU MTRRs don't cover all of memory, losing %lluMB of RAM.\n",
			total_trim_size >> 20);

		if (!changed_by_mtrr_cleanup)
			WARN_ON(1);

		pr_info("update e820 for mtrr\n");
		e820__update_table_print();

		return 1;
	}

	return 0;
}
