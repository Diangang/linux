// SPDX-License-Identifier: GPL-2.0-only
// Copyright 2023 Google LLC
// Author: Ard Biesheuvel <ardb@google.com>

#include <linux/init.h>
#include <linux/libfdt.h>
#include <linux/linkage.h>
#include <linux/types.h>
#include <linux/sizes.h>
#include <linux/string.h>

#include <asm/memory.h>
#include <asm/pgalloc.h>
#include <asm/pgtable.h>
#include <asm/tlbflush.h>

#include "pi.h"

extern void idmap_cpu_replace_ttbr1(phys_addr_t pgdir);

static void __init map_segment(pgd_t *pg_dir, phys_addr_t *pgd, u64 va_offset,
			       void *start, void *end, pgprot_t prot,
			       bool may_use_cont, int root_level)
{
	map_range(pgd, ((u64)start + va_offset) & ~PAGE_OFFSET,
		  ((u64)end + va_offset) & ~PAGE_OFFSET, (u64)start,
		  prot, root_level, (pte_t *)pg_dir, may_use_cont, 0);
}

static void __init map_kernel(u64 va_offset, int root_level)
{
	phys_addr_t pgdp = (phys_addr_t)init_pg_dir + PAGE_SIZE;
	pgprot_t text_prot = PAGE_KERNEL_ROX;
	pgprot_t data_prot = PAGE_KERNEL;

	/*
	 * External debuggers may need to write directly to the text mapping to
	 * install SW breakpoints. Allow this (only) when explicitly requested
	 * with rodata=off.
	 */
	if (arm64_test_sw_feature_override(ARM64_SW_FEATURE_OVERRIDE_RODATA_OFF))
		text_prot = PAGE_KERNEL_EXEC;

	/*
	 * [_stext, _text) isn't executed after boot and contains some
	 * non-executable, unpredictable data, so map it non-executable.
	 */
	map_segment(init_pg_dir, &pgdp, va_offset, _text, _stext, data_prot,
		    false, root_level);
	map_segment(init_pg_dir, &pgdp, va_offset, _stext, _etext, text_prot,
		    true, root_level);
	map_segment(init_pg_dir, &pgdp, va_offset, __start_rodata,
		    __inittext_begin, data_prot, false, root_level);
	map_segment(init_pg_dir, &pgdp, va_offset, __inittext_begin,
		    __inittext_end, text_prot, false, root_level);
	map_segment(init_pg_dir, &pgdp, va_offset, __initdata_begin,
		    __initdata_end, data_prot, false, root_level);
	map_segment(init_pg_dir, &pgdp, va_offset, _data, _end, data_prot,
		    true, root_level);
	dsb(ishst);

	idmap_cpu_replace_ttbr1((phys_addr_t)init_pg_dir);

	/* Copy the root page table to its final location */
	memcpy((void *)swapper_pg_dir + va_offset, init_pg_dir, PAGE_SIZE);
	dsb(ishst);
	idmap_cpu_replace_ttbr1((phys_addr_t)swapper_pg_dir);
}

static void *__init map_fdt(phys_addr_t fdt)
{
	static u8 ptes[INIT_IDMAP_FDT_SIZE] __initdata __aligned(PAGE_SIZE);
	phys_addr_t efdt = fdt + MAX_FDT_SIZE;
	phys_addr_t ptep = (phys_addr_t)ptes; /* We're idmapped when called */

	/*
	 * Map up to MAX_FDT_SIZE bytes, but avoid overlap with
	 * the kernel image.
	 */
	map_range(&ptep, fdt, (u64)_text > fdt ? min((u64)_text, efdt) : efdt,
		  fdt, PAGE_KERNEL, IDMAP_ROOT_LEVEL,
		  (pte_t *)init_idmap_pg_dir, false, 0);
	dsb(ishst);

	return (void *)fdt;
}

/*
 * PI version of the Cavium Eratum 27456 detection, which makes it
 * impossible to use non-global mappings.
 */
asmlinkage void __init early_map_kernel(u64 boot_status, phys_addr_t fdt)
{
	static char const chosen_str[] __initconst = "/chosen";
	u64 pa_base = (u64)&_text;
	int root_level = 0;
	int chosen;
	void *fdt_mapped = map_fdt(fdt);

	/* Clear BSS and the initial page tables */
	memset(__bss_start, 0, (char *)init_pg_end - (char *)__bss_start);

	/* Parse the command line for CPU feature overrides */
	chosen = fdt_path_offset(fdt_mapped, chosen_str);
	init_feature_override(boot_status, fdt_mapped, chosen);

	map_kernel(KIMAGE_VADDR - pa_base, root_level);
}
