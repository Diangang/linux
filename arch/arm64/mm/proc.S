/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Based on arch/arm/mm/proc.S
 *
 * Copyright (C) 2001 Deep Blue Solutions Ltd.
 * Copyright (C) 2012 ARM Ltd.
 * Author: Catalin Marinas <catalin.marinas@arm.com>
 */

#include <linux/init.h>
#include <linux/linkage.h>
#include <linux/pgtable.h>
#include <linux/cfi_types.h>
#include <asm/assembler.h>
#include <asm/asm-offsets.h>
#include <asm/hwcap.h>
#include <asm/kernel-pgtable.h>
#include <asm/pgtable-hwdef.h>
#include <asm/cpufeature.h>
#include <asm/alternative.h>
#include <asm/smp.h>
#include <asm/sysreg.h>

#define TCR_TG_FLAGS	((TCR_EL1_TG0_4K << TCR_EL1_TG0_SHIFT) |\
			 (TCR_EL1_TG1_4K << TCR_EL1_TG1_SHIFT))

#ifdef CONFIG_RANDOMIZE_BASE
#define TCR_KASLR_FLAGS	TCR_EL1_NFD1
#else
#define TCR_KASLR_FLAGS	0
#endif

/* PTWs cacheable, inner/outer WBWA */
#define TCR_CACHE_FLAGS	TCR_IRGN_WBWA | TCR_ORGN_WBWA

#define TCR_MTE_FLAGS 0

#define TCR_IRGN_WBWA	((TCR_EL1_IRGN0_WBWA << TCR_EL1_IRGN0_SHIFT) |\
			 (TCR_EL1_IRGN1_WBWA << TCR_EL1_IRGN1_SHIFT))
#define TCR_ORGN_WBWA	((TCR_EL1_ORGN0_WBWA << TCR_EL1_ORGN0_SHIFT) |\
			 (TCR_EL1_ORGN1_WBWA << TCR_EL1_ORGN1_SHIFT))
#define TCR_SHARED	((TCR_EL1_SH0_INNER << TCR_EL1_SH0_SHIFT) |\
			 (TCR_EL1_SH1_INNER << TCR_EL1_SH1_SHIFT))

/*
 * Default MAIR_EL1. MT_NORMAL_TAGGED is initially mapped as Normal memory and
 * changed during mte_cpu_setup to Normal Tagged if the system supports MTE.
 */
#define MAIR_EL1_SET							\
	(MAIR_ATTRIDX(MAIR_ATTR_DEVICE_nGnRnE, MT_DEVICE_nGnRnE) |	\
	 MAIR_ATTRIDX(MAIR_ATTR_DEVICE_nGnRE, MT_DEVICE_nGnRE) |	\
	 MAIR_ATTRIDX(MAIR_ATTR_NORMAL_NC, MT_NORMAL_NC) |		\
	 MAIR_ATTRIDX(MAIR_ATTR_NORMAL, MT_NORMAL) |			\
	 MAIR_ATTRIDX(MAIR_ATTR_NORMAL, MT_NORMAL_TAGGED))


	.pushsection ".idmap.text", "a"

.macro	__idmap_cpu_set_reserved_ttbr1, tmp1, tmp2
	adrp	\tmp1, reserved_pg_dir
	phys_to_ttbr \tmp2, \tmp1
	offset_ttbr1 \tmp2, \tmp1
	msr	ttbr1_el1, \tmp2
	isb
	tlbi	vmalle1
	dsb	nsh
	isb
.endm

/*
 * void idmap_cpu_replace_ttbr1(phys_addr_t ttbr1)
 *
 * This is the low-level counterpart to cpu_replace_ttbr1, and should not be
 * called by anything else. It can only be executed from a TTBR0 mapping.
 */
SYM_TYPED_FUNC_START(idmap_cpu_replace_ttbr1)
	__idmap_cpu_set_reserved_ttbr1 x1, x3

	offset_ttbr1 x0, x3
	msr	ttbr1_el1, x0
	isb

	ret
SYM_FUNC_END(idmap_cpu_replace_ttbr1)
SYM_FUNC_ALIAS(__pi_idmap_cpu_replace_ttbr1, idmap_cpu_replace_ttbr1)
	.popsection

#ifdef CONFIG_UNMAP_KERNEL_AT_EL0

#define KPTI_NG_PTE_FLAGS	(PTE_ATTRINDX(MT_NORMAL) | PTE_TYPE_PAGE | \
				 PTE_AF | PTE_SHARED | PTE_UXN | PTE_WRITE)

	.pushsection ".idmap.text", "a"

	.macro	pte_to_phys, phys, pte
	and	\phys, \pte, #PTE_ADDR_LOW
	.endm

	.macro	kpti_mk_tbl_ng, type, num_entries
	add	end_\type\()p, cur_\type\()p, #\num_entries * 8
.Ldo_\type:
	ldr	\type, [cur_\type\()p], #8	// Load the entry and advance
	tbz	\type, #0, .Lnext_\type		// Skip invalid and
	tbnz	\type, #11, .Lnext_\type	// non-global entries
	orr	\type, \type, #PTE_NG		// Same bit for blocks and pages
	str	\type, [cur_\type\()p, #-8]	// Update the entry
	.ifnc	\type, pte
	tbnz	\type, #1, .Lderef_\type
	.endif
.Lnext_\type:
	cmp	cur_\type\()p, end_\type\()p
	b.ne	.Ldo_\type
	.endm

	/*
	 * Dereference the current table entry and map it into the temporary
	 * fixmap slot associated with the current level.
	 */
	.macro	kpti_map_pgtbl, type, level
	str	xzr, [temp_pte, #8 * (\level + 2)]	// break before make
	dsb	nshst
	add	pte, temp_pte, #PAGE_SIZE * (\level + 2)
	lsr	pte, pte, #12
	tlbi	vaae1, pte
	dsb	nsh
	isb

	phys_to_pte pte, cur_\type\()p
	add	cur_\type\()p, temp_pte, #PAGE_SIZE * (\level + 2)
	orr	pte, pte, pte_flags
	str	pte, [temp_pte, #8 * (\level + 2)]
	dsb	nshst
	.endm

/*
 * void __kpti_install_ng_mappings(int cpu, int num_secondaries, phys_addr_t temp_pgd,
 *				   unsigned long temp_pte_va)
 *
 * Called exactly once from stop_machine context by each CPU found during boot.
 */
SYM_TYPED_FUNC_START(idmap_kpti_install_ng_mappings)
	cpu		.req	w0
	temp_pte	.req	x0
	num_cpus	.req	w1
	pte_flags	.req	x1
	temp_pgd_phys	.req	x2
	swapper_ttb	.req	x3
	flag_ptr	.req	x4
	cur_pgdp	.req	x5
	end_pgdp	.req	x6
	pgd		.req	x7
	cur_pudp	.req	x8
	end_pudp	.req	x9
	cur_pmdp	.req	x11
	end_pmdp	.req	x12
	cur_ptep	.req	x14
	end_ptep	.req	x15
	pte		.req	x16
	valid		.req	x17
	cur_p4dp	.req	x19
	end_p4dp	.req	x20

	mov	x5, x3				// preserve temp_pte arg
	mrs	swapper_ttb, ttbr1_el1
	adr_l	flag_ptr, idmap_kpti_bbml2_flag

	cbnz	cpu, __idmap_kpti_secondary


	/* We're the boot CPU. Wait for the others to catch up */
	sevl
1:	wfe
	ldaxr	w17, [flag_ptr]
	eor	w17, w17, num_cpus
	cbnz	w17, 1b

	/* Switch to the temporary page tables on this CPU only */
	__idmap_cpu_set_reserved_ttbr1 x8, x9
	offset_ttbr1 temp_pgd_phys, x8
	msr	ttbr1_el1, temp_pgd_phys
	isb

	mov	temp_pte, x5
	mov_q	pte_flags, KPTI_NG_PTE_FLAGS

	/* Everybody is enjoying the idmap, so we can rewrite swapper. */


	/* PGD */
	adrp		cur_pgdp, swapper_pg_dir
	kpti_map_pgtbl	pgd, -1
	kpti_mk_tbl_ng	pgd, PTRS_PER_PGD

	/* Ensure all the updated entries are visible to secondary CPUs */
	dsb	ishst

	/* We're done: fire up swapper_pg_dir again */
	__idmap_cpu_set_reserved_ttbr1 x8, x9
	msr	ttbr1_el1, swapper_ttb
	isb

	/* Set the flag to zero to indicate that we're all done */
	str	wzr, [flag_ptr]
	ret

.Lderef_pgd:
	/* P4D */
	p4d		.req	pgd
	.set		.Lnext_p4d, .Lnext_pgd

.Lderef_p4d:
	/* PUD */
	pud		.req	x10
	pte_to_phys	cur_pudp, p4d
	kpti_map_pgtbl	pud, 1
	kpti_mk_tbl_ng	pud, PTRS_PER_PUD
	b		.Lnext_p4d

.Lderef_pud:
	/* PMD */
	pmd		.req	x13
	pte_to_phys	cur_pmdp, pud
	kpti_map_pgtbl	pmd, 2
	kpti_mk_tbl_ng	pmd, PTRS_PER_PMD
	b		.Lnext_pud

.Lderef_pmd:
	/* PTE */
	pte_to_phys	cur_ptep, pmd
	kpti_map_pgtbl	pte, 3
	kpti_mk_tbl_ng	pte, PTRS_PER_PTE
	b		.Lnext_pmd

	.unreq	cpu
	.unreq	temp_pte
	.unreq	num_cpus
	.unreq	pte_flags
	.unreq	temp_pgd_phys
	.unreq	cur_pgdp
	.unreq	end_pgdp
	.unreq	pgd
	.unreq	cur_pudp
	.unreq	end_pudp
	.unreq	pud
	.unreq	cur_pmdp
	.unreq	end_pmdp
	.unreq	pmd
	.unreq	cur_ptep
	.unreq	end_ptep
	.unreq	pte
	.unreq	valid
	.unreq	cur_p4dp
	.unreq	end_p4dp
	.unreq	p4d

	/* Secondary CPUs end up here */
__idmap_kpti_secondary:
	/* Uninstall swapper before surgery begins */
	__idmap_cpu_set_reserved_ttbr1 x16, x17
	b scondary_cpu_wait

	.unreq	swapper_ttb
	.unreq	flag_ptr
SYM_FUNC_END(idmap_kpti_install_ng_mappings)
	.popsection
#endif

	.pushsection ".idmap.text", "a"
SYM_TYPED_FUNC_START(wait_linear_map_split_to_ptes)
	/* Must be same registers as in idmap_kpti_install_ng_mappings */
	swapper_ttb	.req	x3
	flag_ptr	.req	x4

	mrs     swapper_ttb, ttbr1_el1
	adr_l   flag_ptr, idmap_kpti_bbml2_flag
	__idmap_cpu_set_reserved_ttbr1 x16, x17

scondary_cpu_wait:
	/* Increment the flag to let the boot CPU we're ready */
1:	ldxr	w16, [flag_ptr]
	add	w16, w16, #1
	stxr	w17, w16, [flag_ptr]
	cbnz	w17, 1b

	/* Wait for the boot CPU to finish messing around with swapper */
	sevl
1:	wfe
	ldxr	w16, [flag_ptr]
	cbnz	w16, 1b

	/* All done, act like nothing happened */
	msr	ttbr1_el1, swapper_ttb
	isb
	ret

	.unreq	swapper_ttb
	.unreq	flag_ptr
SYM_FUNC_END(wait_linear_map_split_to_ptes)
	.popsection

/*
 *	__cpu_setup
 *
 *	Initialise the processor for turning the MMU on.
 *
 * Output:
 *	Return in x0 the value of the SCTLR_EL1 register.
 */
	.pushsection ".idmap.text", "a"
SYM_FUNC_START(__cpu_setup)
	tlbi	vmalle1				// Invalidate local TLB
	dsb	nsh

	msr	cpacr_el1, xzr			// Reset cpacr_el1
	mov	x1, MDSCR_EL1_TDCC		// Reset mdscr_el1 and disable
	msr	mdscr_el1, x1			// access to the DCC from EL0
	reset_pmuserenr_el0 x1			// Disable PMU access from EL0
	reset_amuserenr_el0 x1			// Disable AMU access from EL0

	/*
	 * Default values for VMSA control registers. These will be adjusted
	 * below depending on detected CPU features.
	 */
	mair	.req	x17
	tcr	.req	x16
	tcr2	.req	x15
	mov_q	mair, MAIR_EL1_SET
	mov_q	tcr, TCR_T0SZ(IDMAP_VA_BITS) | TCR_T1SZ(VA_BITS_MIN) | TCR_CACHE_FLAGS | \
		     TCR_SHARED | TCR_TG_FLAGS | TCR_KASLR_FLAGS | TCR_EL1_AS | \
		     TCR_EL1_TBI0 | TCR_EL1_A1 | TCR_MTE_FLAGS
	mov	tcr2, xzr

	tcr_clear_errata_bits tcr, x9, x5


	/*
	 * Set the IPS bits in TCR_EL1.
	 */
	tcr_compute_pa_size tcr, #TCR_EL1_IPS_SHIFT, x5, x6
	msr	mair_el1, mair
	msr	tcr_el1, tcr

	mrs_s	x1, SYS_ID_AA64MMFR3_EL1
	ubfx	x1, x1, #ID_AA64MMFR3_EL1_S1PIE_SHIFT, #4
	cbz	x1, .Lskip_indirection

	mov_q	x0, PIE_E0_ASM
	msr	REG_PIRE0_EL1, x0
	mov_q	x0, PIE_E1_ASM
	msr	REG_PIR_EL1, x0

	orr	tcr2, tcr2, TCR2_EL1_PIE

.Lskip_indirection:

	mrs_s	x1, SYS_ID_AA64MMFR3_EL1
	ubfx	x1, x1, #ID_AA64MMFR3_EL1_TCRX_SHIFT, #4
	cbz	x1, 1f
	msr	REG_TCR2_EL1, tcr2
1:

	/*
	 * Prepare SCTLR
	 */
	mov_q	x0, INIT_SCTLR_EL1_MMU_ON
	ret					// return to head.S

	.unreq	mair
	.unreq	tcr
	.unreq	tcr2
SYM_FUNC_END(__cpu_setup)
