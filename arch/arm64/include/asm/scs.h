/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_SCS_H
#define _ASM_SCS_H

#ifdef __ASSEMBLER__

#include <asm/asm-offsets.h>
#include <asm/sysreg.h>

	.macro scs_load_current_base
	.endm

	.macro scs_load_current
	.endm

	.macro scs_save tsk
	.endm


#else

#include <linux/scs.h>
#include <asm/cpufeature.h>

static inline void dynamic_scs_init(void) {}

enum {
	EDYNSCS_INVALID_CIE_HEADER		= 1,
	EDYNSCS_INVALID_CIE_SDATA_SIZE		= 2,
	EDYNSCS_INVALID_FDE_AUGM_DATA_SIZE	= 3,
	EDYNSCS_INVALID_CFA_OPCODE		= 4,
};

int __pi_scs_patch(const u8 eh_frame[], int size, bool skip_dry_run);

#endif /* __ASSEMBLER__ */

#endif /* _ASM_SCS_H */
