/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __ASM_CPUCAPS_H
#define __ASM_CPUCAPS_H

#include <asm/cpucap-defs.h>

#ifndef __ASSEMBLER__
#include <linux/types.h>
/*
 * Check whether a cpucap is possible at compiletime.
 */
static __always_inline bool
cpucap_is_possible(const unsigned int cap)
{
	compiletime_assert(__builtin_constant_p(cap),
			   "cap must be a constant");
	compiletime_assert(cap < ARM64_NCAPS,
			   "cap must be < ARM64_NCAPS");

	switch (cap) {
	case ARM64_HAS_EPAN:
		return IS_ENABLED(CONFIG_ARM64_EPAN);
	case ARM64_SVE:
		return IS_ENABLED(CONFIG_ARM64_SVE);
	case ARM64_SME:
	case ARM64_SME2:
	case ARM64_SME_FA64:
		return IS_ENABLED(CONFIG_ARM64_SME);
	case ARM64_HAS_CNP:
		return IS_ENABLED(CONFIG_ARM64_CNP);
	case ARM64_HAS_GIC_PRIO_MASKING:
		return 0;
	case ARM64_MTE:
		return false;
	case ARM64_BTI:
		return IS_ENABLED(CONFIG_ARM64_BTI);
	case ARM64_HAS_TLB_RANGE:
		return IS_ENABLED(CONFIG_ARM64_TLB_RANGE);
	case ARM64_HAS_S1POE:
		return IS_ENABLED(CONFIG_ARM64_POE);
	case ARM64_HAS_GCS:
		return IS_ENABLED(CONFIG_ARM64_GCS);
	case ARM64_HAFT:
		return IS_ENABLED(CONFIG_ARM64_HAFT);
	case ARM64_UNMAP_KERNEL_AT_EL0:
		return IS_ENABLED(CONFIG_UNMAP_KERNEL_AT_EL0);
	case ARM64_MPAM:
		/*
		 * KVM MPAM support doesn't rely on the host kernel supporting MPAM.
		*/
		return true;
	case ARM64_HAS_LSUI:
		return false;
	}

	return true;
}
#endif /* __ASSEMBLER__ */

#endif /* __ASM_CPUCAPS_H */
