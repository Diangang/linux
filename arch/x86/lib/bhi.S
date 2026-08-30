/* SPDX-License-Identifier: GPL-2.0 */

#include <linux/linkage.h>
#include <asm/unwind_hints.h>
#include <asm/nospec-branch.h>

/*
 * Notably, the FineIBT preamble calling these will have ZF set and eax zero.
 *
 * The very last element is in fact larger than 32 bytes, but since its the
 * last element, this does not matter,
 *
 * There are 2 #UD sites, located between 0,1-2,3 and 4,5-6,7 such that they
 * can be reached using Jcc.d8, these elements (1 and 5) have sufficiently
 * big alignment holes for this to not stagger the array.
 */

.pushsection .noinstr.text, "ax"

	.align 32
SYM_CODE_START(__bhi_args)


	.align 32
SYM_INNER_LABEL(__bhi_args_end, SYM_L_GLOBAL)
	ANNOTATE_NOENDBR
	nop /* Work around toolchain+objtool quirk */
SYM_CODE_END(__bhi_args)

.popsection
