/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Based on arch/arm/include/asm/traps.h
 *
 * Copyright (C) 2012 ARM Ltd.
 */
#ifndef __ASM_TRAP_H
#define __ASM_TRAP_H

#include <linux/list.h>
#include <asm/esr.h>
#include <asm/ptrace.h>
#include <asm/sections.h>

static inline bool
try_emulate_armv8_deprecated(struct pt_regs *regs, u32 insn)
{
	return false;
}

void force_signal_inject(int signal, int code, unsigned long address, unsigned long err);
void arm64_notify_segfault(unsigned long addr);
void arm64_force_sig_fault(int signo, int code, unsigned long far, const char *str);
void arm64_force_sig_fault_pkey(unsigned long far, const char *str, int pkey);
void arm64_force_sig_mceerr(int code, unsigned long far, short lsb, const char *str);
void arm64_force_sig_ptrace_errno_trap(int errno, unsigned long far, const char *str);

int bug_brk_handler(struct pt_regs *regs, unsigned long esr);
int reserved_fault_brk_handler(struct pt_regs *regs, unsigned long esr);
int ubsan_brk_handler(struct pt_regs *regs, unsigned long esr);

int early_brk64(unsigned long addr, unsigned long esr, struct pt_regs *regs);
void dump_kernel_instr(unsigned long kaddr);

/*
 * Move regs->pc to next instruction and do necessary setup before it
 * is executed.
 */
void arm64_skip_faulting_instruction(struct pt_regs *regs, unsigned long size);

static inline int __in_irqentry_text(unsigned long ptr)
{
	return ptr >= (unsigned long)&__irqentry_text_start &&
	       ptr < (unsigned long)&__irqentry_text_end;
}

static inline int in_entry_text(unsigned long ptr)
{
	return ptr >= (unsigned long)&__entry_text_start &&
	       ptr < (unsigned long)&__entry_text_end;
}

static inline void arm64_mops_reset_regs(struct user_pt_regs *regs, unsigned long esr)
{
	bool wrong_option = esr & ESR_ELx_MOPS_ISS_WRONG_OPTION;
	bool option_a = esr & ESR_ELx_MOPS_ISS_OPTION_A;
	int dstreg = ESR_ELx_MOPS_ISS_DESTREG(esr);
	int srcreg = ESR_ELx_MOPS_ISS_SRCREG(esr);
	int sizereg = ESR_ELx_MOPS_ISS_SIZEREG(esr);
	unsigned long dst, size;

	dst = regs->regs[dstreg];
	size = regs->regs[sizereg];

	/*
	 * Put the registers back in the original format suitable for a
	 * prologue instruction, using the generic return routine from the
	 * Arm ARM (DDI 0487I.a) rules CNTMJ and MWFQH.
	 */
	if (esr & ESR_ELx_MOPS_ISS_MEM_INST) {
		/* SET* instruction */
		if (option_a ^ wrong_option) {
			/* Format is from Option A; forward set */
			regs->regs[dstreg] = dst + size;
			regs->regs[sizereg] = -size;
		}
	} else {
		/* CPY* instruction */
		unsigned long src = regs->regs[srcreg];
		if (!(option_a ^ wrong_option)) {
			/* Format is from Option B */
			if (regs->pstate & PSR_N_BIT) {
				/* Backward copy */
				regs->regs[dstreg] = dst - size;
				regs->regs[srcreg] = src - size;
			}
		} else {
			/* Format is from Option A */
			if (size & BIT(63)) {
				/* Forward copy */
				regs->regs[dstreg] = dst + size;
				regs->regs[srcreg] = src + size;
				regs->regs[sizereg] = -size;
			}
		}
	}

	if (esr & ESR_ELx_MOPS_ISS_FROM_EPILOGUE)
		regs->pc -= 8;
	else
		regs->pc -= 4;
}
#endif
