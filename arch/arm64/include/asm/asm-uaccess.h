/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __ASM_ASM_UACCESS_H
#define __ASM_ASM_UACCESS_H

#include <asm/alternative-macros.h>
#include <asm/asm-extable.h>
#include <asm/assembler.h>
#include <asm/kernel-pgtable.h>
#include <asm/mmu.h>
#include <asm/sysreg.h>

/*
 * User access enabling/disabling macros.
 */
	.macro	uaccess_ttbr0_disable, tmp1, tmp2
	.endm

	.macro	uaccess_ttbr0_enable, tmp1, tmp2, tmp3
	.endm

#define USER(l, x...)				\
9999:	x;					\
	_asm_extable_uaccess	9999b, l

#define USER_CPY(l, uaccess_is_write, x...)	\
9999:	x;					\
	_asm_extable_uaccess_cpy 9999b, l, uaccess_is_write

/*
 * Generate the assembly for LDTR/STTR with exception table entries.
 * This is complicated as there is no post-increment or pair versions of the
 * unprivileged instructions, and USER() only works for single instructions.
 */
	.macro user_ldp l, reg1, reg2, addr, post_inc
8888:		ldtr	\reg1, [\addr];
8889:		ldtr	\reg2, [\addr, #8];
		add	\addr, \addr, \post_inc;

		_asm_extable_uaccess	8888b, \l;
		_asm_extable_uaccess	8889b, \l;
	.endm

	.macro user_stp l, reg1, reg2, addr, post_inc
8888:		sttr	\reg1, [\addr];
8889:		sttr	\reg2, [\addr, #8];
		add	\addr, \addr, \post_inc;

		_asm_extable_uaccess	8888b,\l;
		_asm_extable_uaccess	8889b,\l;
	.endm

	.macro user_ldst l, inst, reg, addr, post_inc
8888:		\inst		\reg, [\addr];
		add		\addr, \addr, \post_inc;

		_asm_extable_uaccess	8888b, \l;
	.endm
#endif
