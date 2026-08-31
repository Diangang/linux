/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Based on arch/arm/include/asm/tlbflush.h
 *
 * Copyright (C) 1999-2003 Russell King
 * Copyright (C) 2012 ARM Ltd.
 */
#ifndef __ASM_TLBFLUSH_H
#define __ASM_TLBFLUSH_H

#ifndef __ASSEMBLER__

#include <linux/bitfield.h>
#include <linux/mm_types.h>
#include <linux/sched.h>
#include <linux/mmu_notifier.h>
#include <asm/cputype.h>
#include <asm/mmu.h>

/*
 * Raw TLBI operations.
 *
 * Where necessary, use the __tlbi() macro to avoid asm()
 * boilerplate. Drivers and most kernel code should use the TLB
 * management routines in preference to the macro below.
 *
 * The macro can be used as __tlbi(op) or __tlbi(op, arg), depending
 * on whether a particular TLBI operation takes an argument or
 * not. The macros handles invoking the asm with or without the
 * register argument as appropriate.
 */
#define __TLBI_0(op, arg) asm (ARM64_ASM_PREAMBLE			       \
			       "tlbi " #op "\n"				       \
			    : : )

#define __TLBI_1(op, arg) asm (ARM64_ASM_PREAMBLE			       \
			       "tlbi " #op ", %x0\n"			       \
			    : : "rZ" (arg))

#define __TLBI_N(op, arg, n, ...) __TLBI_##n(op, arg)

#define __tlbi(op, ...)		__TLBI_N(op, ##__VA_ARGS__, 1, 0)

#define __tlbi_user(op, arg) do {						\
	if (arm64_kernel_unmapped_at_el0())					\
		__tlbi(op, (arg) | USER_ASID_FLAG);				\
} while (0)

/* This macro creates a properly formatted VA operand for the TLBI */
#define __TLBI_VADDR(addr, asid)				\
	({							\
		unsigned long __ta = (addr) >> 12;		\
		__ta &= GENMASK_ULL(43, 0);			\
		__ta |= (unsigned long)(asid) << 48;		\
		__ta;						\
	})

/*
 * Get translation granule of the system, which is decided by
 * PAGE_SIZE.  Used by TTL.
 *  - 4KB	: 1
 *  - 16KB	: 2
 *  - 64KB	: 3
 */
#define TLBI_TTL_TG_4K		1
#define TLBI_TTL_TG_16K		2
#define TLBI_TTL_TG_64K		3

static inline unsigned long get_trans_granule(void)
{
	switch (PAGE_SIZE) {
	case SZ_4K:
		return TLBI_TTL_TG_4K;
	case SZ_16K:
		return TLBI_TTL_TG_16K;
	case SZ_64K:
		return TLBI_TTL_TG_64K;
	default:
		return 0;
	}
}

/*
 * Level-based TLBI operations.
 *
 * When ARMv8.4-TTL exists, TLBI operations take an additional hint for
 * the level at which the invalidation must take place. If the level is
 * wrong, no invalidation may take place. In the case where the level
 * cannot be easily determined, the value TLBI_TTL_UNKNOWN will perform
 * a non-hinted invalidation. Any provided level outside the hint range
 * will also cause fall-back to non-hinted invalidation.
 *
 * For Stage-2 invalidation, use the level values provided to that effect
 * in asm/stage2_pgtable.h.
 */
#define TLBI_TTL_MASK		GENMASK_ULL(47, 44)

#define TLBI_TTL_UNKNOWN	INT_MAX

typedef void (*tlbi_op)(u64 arg);

static __always_inline void vae1is(u64 arg)
{
	__tlbi(vae1is, arg);
	__tlbi_user(vae1is, arg);
}

static __always_inline void vae2is(u64 arg)
{
	__tlbi(vae2is, arg);
}

static __always_inline void vale1(u64 arg)
{
	__tlbi(vale1, arg);
	__tlbi_user(vale1, arg);
}

static __always_inline void vale1is(u64 arg)
{
	__tlbi(vale1is, arg);
	__tlbi_user(vale1is, arg);
}

static __always_inline void vale2is(u64 arg)
{
	__tlbi(vale2is, arg);
}

static __always_inline void vaale1is(u64 arg)
{
	__tlbi(vaale1is, arg);
}

static __always_inline void ipas2e1(u64 arg)
{
	__tlbi(ipas2e1, arg);
}

static __always_inline void ipas2e1is(u64 arg)
{
	__tlbi(ipas2e1is, arg);
}

static __always_inline void __tlbi_level_asid(tlbi_op op, u64 addr, u32 level,
					      u16 asid)
{
	u64 arg = __TLBI_VADDR(addr, asid);

	if (alternative_has_cap_unlikely(ARM64_HAS_ARMv8_4_TTL) && level <= 3) {
		u64 ttl = level | (get_trans_granule() << 2);

		FIELD_MODIFY(TLBI_TTL_MASK, &arg, ttl);
	}

	op(arg);
}

static inline void __tlbi_level(tlbi_op op, u64 addr, u32 level)
{
	__tlbi_level_asid(op, addr, level, 0);
}

/*
 * This macro creates a properly formatted VA operand for the TLB RANGE. The
 * value bit assignments are:
 *
 * +----------+------+-------+-------+-------+----------------------+
 * |   ASID   |  TG  | SCALE |  NUM  |  TTL  |        BADDR         |
 * +-----------------+-------+-------+-------+----------------------+
 * |63      48|47  46|45   44|43   39|38   37|36                   0|
 *
 * The address range is determined by below formula: [BADDR, BADDR + (NUM + 1) *
 * 2^(5*SCALE + 1) * PAGESIZE)
 *
 * Note that the first argument, baddr, is pre-shifted; If LPA2 is in use, BADDR
 * holds addr[52:16]. Else BADDR holds page number. See for example ARM DDI
 * 0487J.a section C5.5.60 "TLBI VAE1IS, TLBI VAE1ISNXS, TLB Invalidate by VA,
 * EL1, Inner Shareable".
 *
 */
/*
 * Complete broadcast TLB maintenance issued by the host which invalidates
 * stage 1 information in the host's own translation regime.
 */
static inline void __tlbi_sync_s1ish(struct mm_struct *mm)
{
	dsb(ish);
}

static inline void __tlbi_sync_s1ish_batch(struct arch_tlbflush_unmap_batch *batch)
{
	dsb(ish);
}

static inline void __tlbi_sync_s1ish_kernel(void)
{
	dsb(ish);
}

/*
 * Complete broadcast TLB maintenance issued by hyp code which invalidates
 * stage 1 translation information in any translation regime.
 */
static inline void __tlbi_sync_s1ish_hyp(void)
{
	dsb(ish);
}

/*
 *	TLB Invalidation
 *	================
 *
 * 	This header file implements the low-level TLB invalidation routines
 *	(sometimes referred to as "flushing" in the kernel) for arm64.
 *
 *	Every invalidation operation uses the following template:
 *
 *	DSB ISHST	// Ensure prior page-table updates have completed
 *	TLBI ...	// Invalidate the TLB
 *	DSB ISH		// Ensure the TLB invalidation has completed
 *      if (invalidated kernel mappings)
 *		ISB	// Discard any instructions fetched from the old mapping
 *
 *
 *	The following functions form part of the "core" TLB invalidation API,
 *	as documented in Documentation/core-api/cachetlb.rst:
 *
 *	flush_tlb_all()
 *		Invalidate the entire TLB (kernel + user) on all CPUs
 *
 *	flush_tlb_mm(mm)
 *		Invalidate an entire user address space on all CPUs.
 *		The 'mm' argument identifies the ASID to invalidate.
 *
 *	flush_tlb_range(vma, start, end)
 *		Invalidate the virtual-address range '[start, end)' on all
 *		CPUs for the user address space corresponding to 'vma->mm'.
 *		Note that this operation also invalidates any walk-cache
 *		entries associated with translations for the specified address
 *		range.
 *
 *	flush_tlb_kernel_range(start, end)
 *		Same as flush_tlb_range(..., start, end), but applies to
 * 		kernel mappings rather than a particular user address space.
 *		Whilst not explicitly documented, this function is used when
 *		unmapping pages from vmalloc/io space.
 *
 *	flush_tlb_page(vma, addr)
 *		Equivalent to __flush_tlb_page(..., flags=TLBF_NONE)
 *
 *
 *	Next, we have some undocumented invalidation routines that you probably
 *	don't want to call unless you know what you're doing:
 *
 *	local_flush_tlb_all()
 *		Same as flush_tlb_all(), but only applies to the calling CPU.
 *
 *	__flush_tlb_kernel_pgtable(addr)
 *		Invalidate a single kernel mapping for address 'addr' on all
 *		CPUs, ensuring that any walk-cache entries associated with the
 *		translation are also invalidated.
 *
 *	__flush_tlb_range(vma, start, end, stride, tlb_level, flags)
 *		Invalidate the virtual-address range '[start, end)' on all
 *		CPUs for the user address space corresponding to 'vma->mm'.
 *		The invalidation operations are issued at a granularity
 *		determined by 'stride'. tlb_level is the level at
 *		which the invalidation must take place. If the level is wrong,
 *		no invalidation may take place. In the case where the level
 *		cannot be easily determined, the value TLBI_TTL_UNKNOWN will
 *		perform a non-hinted invalidation. flags may be TLBF_NONE (0) or
 *		any combination of TLBF_NOWALKCACHE (elide eviction of walk
 *		cache entries), TLBF_NONOTIFY (don't call mmu notifiers),
 *		TLBF_NOSYNC (don't issue trailing dsb) and TLBF_NOBROADCAST
 *		(only perform the invalidation for the local cpu).
 *
 *	__flush_tlb_page(vma, addr, flags)
 *		Invalidate a single user mapping for address 'addr' in the
 *		address space corresponding to 'vma->mm'.  Note that this
 *		operation only invalidates a single level 3 page-table entry
 *		and therefore does not affect any walk-caches. flags may contain
 *		any combination of TLBF_NONOTIFY (don't call mmu notifiers),
 *		TLBF_NOSYNC (don't issue trailing dsb) and TLBF_NOBROADCAST
 *		(only perform the invalidation for the local cpu).
 *
 *	Finally, take a look at asm/tlb.h to see how tlb_flush() is implemented
 *	on top of these routines, since that is our interface to the mmu_gather
 *	API as used by munmap() and friends.
 */
static inline void local_flush_tlb_all(void)
{
	dsb(nshst);
	__tlbi(vmalle1);
	dsb(nsh);
	isb();
}

static inline void flush_tlb_all(void)
{
	dsb(ishst);
	__tlbi(vmalle1is);
	__tlbi_sync_s1ish_kernel();
	isb();
}

static inline void flush_tlb_mm(struct mm_struct *mm)
{
	unsigned long asid;

	dsb(ishst);
	asid = __TLBI_VADDR(0, ASID(mm));
	__tlbi(aside1is, asid);
	__tlbi_user(aside1is, asid);
	__tlbi_sync_s1ish(mm);
	mmu_notifier_arch_invalidate_secondary_tlbs(mm, 0, -1UL);
}

static inline bool arch_tlbbatch_should_defer(struct mm_struct *mm)
{
	return true;
}

/*
 * To support TLB batched flush for multiple pages unmapping, we only send
 * the TLBI for each page in arch_tlbbatch_add_pending() and wait for the
 * completion at the end in arch_tlbbatch_flush(). Since we've already issued
 * TLBI for each page so only a DSB is needed to synchronise its effect on the
 * other CPUs.
 *
 * This will save the time waiting on DSB comparing issuing a TLBI;DSB sequence
 * for each page.
 */
static inline void arch_tlbbatch_flush(struct arch_tlbflush_unmap_batch *batch)
{
	__tlbi_sync_s1ish_batch(batch);
}

/*
 * This is meant to avoid soft lock-ups on large TLB flushing ranges and not
 * necessarily a performance improvement.
 */
#define MAX_DVM_OPS	PTRS_PER_PTE

static __always_inline void __flush_tlb_range_op(tlbi_op lop,
						 u64 start, size_t pages,
						 u64 stride, u16 asid,
						 u32 level)
{
	u64 addr = start, end = start + pages * PAGE_SIZE;

	while (addr != end) {
		__tlbi_level_asid(lop, addr, level, asid);
		addr += stride;
	}
}

#define __flush_s1_tlb_range_op(op, start, pages, stride, asid, tlb_level) \
	__flush_tlb_range_op(op, start, pages, stride, asid, tlb_level)

#define __flush_s2_tlb_range_op(op, start, pages, stride, tlb_level) \
	__flush_tlb_range_op(op, start, pages, stride, 0, tlb_level)

static inline bool __flush_tlb_range_limit_excess(unsigned long pages,
						  unsigned long stride)
{
	return pages >= (MAX_DVM_OPS * stride) >> PAGE_SHIFT;
}

typedef unsigned __bitwise tlbf_t;

/* No special behaviour. */
#define TLBF_NONE		((__force tlbf_t)0)

/* Invalidate tlb entries only, leaving the page table walk cache intact. */
#define TLBF_NOWALKCACHE	((__force tlbf_t)BIT(0))

/* Skip the trailing dsb after issuing tlbi. */
#define TLBF_NOSYNC		((__force tlbf_t)BIT(1))

/* Suppress tlb notifier callbacks for this flush operation. */
#define TLBF_NONOTIFY		((__force tlbf_t)BIT(2))

/* Perform the tlbi locally without broadcasting to other CPUs. */
#define TLBF_NOBROADCAST	((__force tlbf_t)BIT(3))

static __always_inline void __do_flush_tlb_range(struct vm_area_struct *vma,
					unsigned long start, unsigned long end,
					unsigned long stride, int tlb_level,
					tlbf_t flags)
{
	struct mm_struct *mm = vma->vm_mm;
	unsigned long asid, pages;

	pages = (end - start) >> PAGE_SHIFT;

	if (__flush_tlb_range_limit_excess(pages, stride)) {
		flush_tlb_mm(mm);
		return;
	}

	if (!(flags & TLBF_NOBROADCAST))
		dsb(ishst);
	else
		dsb(nshst);

	asid = ASID(mm);

	switch (flags & (TLBF_NOWALKCACHE | TLBF_NOBROADCAST)) {
	case TLBF_NONE:
		__flush_s1_tlb_range_op(vae1is, start, pages, stride,
					asid, tlb_level);
		break;
	case TLBF_NOWALKCACHE:
		__flush_s1_tlb_range_op(vale1is, start, pages, stride,
					asid, tlb_level);
		break;
	case TLBF_NOBROADCAST:
		/* Combination unused */
		BUG();
		break;
	case TLBF_NOWALKCACHE | TLBF_NOBROADCAST:
		__flush_s1_tlb_range_op(vale1, start, pages, stride,
					asid, tlb_level);
		break;
	}

	if (!(flags & TLBF_NONOTIFY))
		mmu_notifier_arch_invalidate_secondary_tlbs(mm, start, end);

	if (!(flags & TLBF_NOSYNC)) {
		if (!(flags & TLBF_NOBROADCAST))
			__tlbi_sync_s1ish(mm);
		else
			dsb(nsh);
	}
}

static inline void __flush_tlb_range(struct vm_area_struct *vma,
				     unsigned long start, unsigned long end,
				     unsigned long stride, int tlb_level,
				     tlbf_t flags)
{
	start = round_down(start, stride);
	end = round_up(end, stride);
	__do_flush_tlb_range(vma, start, end, stride, tlb_level, flags);
}

static inline void flush_tlb_range(struct vm_area_struct *vma,
				   unsigned long start, unsigned long end)
{
	/*
	 * We cannot use leaf-only invalidation here, since we may be invalidating
	 * table entries as part of collapsing hugepages or moving page tables.
	 * Set the tlb_level to TLBI_TTL_UNKNOWN because we can not get enough
	 * information here.
	 */
	__flush_tlb_range(vma, start, end, PAGE_SIZE, TLBI_TTL_UNKNOWN, TLBF_NONE);
}

static inline void __flush_tlb_page(struct vm_area_struct *vma,
				    unsigned long uaddr, tlbf_t flags)
{
	unsigned long start = round_down(uaddr, PAGE_SIZE);
	unsigned long end = start + PAGE_SIZE;

	__do_flush_tlb_range(vma, start, end, PAGE_SIZE, 3,
			     TLBF_NOWALKCACHE | flags);
}

static inline void flush_tlb_page(struct vm_area_struct *vma,
				  unsigned long uaddr)
{
	__flush_tlb_page(vma, uaddr, TLBF_NONE);
}

static inline void flush_tlb_kernel_range(unsigned long start, unsigned long end)
{
	const unsigned long stride = PAGE_SIZE;
	unsigned long pages;

	start = round_down(start, stride);
	end = round_up(end, stride);
	pages = (end - start) >> PAGE_SHIFT;

	if (__flush_tlb_range_limit_excess(pages, stride)) {
		flush_tlb_all();
		return;
	}

	dsb(ishst);
	__flush_s1_tlb_range_op(vaale1is, start, pages, stride, 0,
				TLBI_TTL_UNKNOWN);
	__tlbi_sync_s1ish_kernel();
	isb();
}

/*
 * Used to invalidate the TLB (walk caches) corresponding to intermediate page
 * table levels (pgd/pud/pmd).
 */
static inline void __flush_tlb_kernel_pgtable(unsigned long kaddr)
{
	unsigned long addr = __TLBI_VADDR(kaddr, 0);

	dsb(ishst);
	__tlbi(vaae1is, addr);
	__tlbi_sync_s1ish_kernel();
	isb();
}

static inline void arch_tlbbatch_add_pending(struct arch_tlbflush_unmap_batch *batch,
		struct mm_struct *mm, unsigned long start, unsigned long end)
{
	struct vm_area_struct vma = { .vm_mm = mm, .vm_flags = 0 };

	__flush_tlb_range(&vma, start, end, PAGE_SIZE, 3,
			  TLBF_NOWALKCACHE | TLBF_NOSYNC);
}

static inline bool __pte_flags_need_flush(ptdesc_t oldval, ptdesc_t newval)
{
	ptdesc_t diff = oldval ^ newval;

	/* invalid to valid transition requires no flush */
	if (!(oldval & PTE_VALID))
		return false;

	/* Transition in the SW bits requires no flush */
	diff &= ~PTE_SWBITS_MASK;

	return diff;
}

static inline bool pte_needs_flush(pte_t oldpte, pte_t newpte)
{
	return __pte_flags_need_flush(pte_val(oldpte), pte_val(newpte));
}
#define pte_needs_flush pte_needs_flush

static inline bool huge_pmd_needs_flush(pmd_t oldpmd, pmd_t newpmd)
{
	return __pte_flags_need_flush(pmd_val(oldpmd), pmd_val(newpmd));
}
#define huge_pmd_needs_flush huge_pmd_needs_flush

#undef __tlbi_user
#undef __TLBI_VADDR
#endif

#endif
