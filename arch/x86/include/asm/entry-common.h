/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _ASM_X86_ENTRY_COMMON_H
#define _ASM_X86_ENTRY_COMMON_H

#include <linux/randomize_kstack.h>

#include <asm/nospec-branch.h>
#include <asm/fpu/api.h>
#include <asm/fred.h>

/* Check that the stack and regs on entry from user mode are sane. */
static __always_inline void arch_enter_from_user_mode(struct pt_regs *regs)
{
}
#define arch_enter_from_user_mode arch_enter_from_user_mode

static inline void arch_exit_work(unsigned long ti_work)
{
	if (unlikely(ti_work & _TIF_NEED_FPU_LOAD))
		switch_fpu_return();
}

static inline void arch_exit_to_user_mode_prepare(struct pt_regs *regs,
						  unsigned long ti_work)
{
	fpregs_assert_state_consistent();

	if (unlikely(ti_work))
		arch_exit_work(ti_work);

	fred_update_rsp0();


	/* Avoid unnecessary reads of 'x86_ibpb_exit_to_user' */
	if (cpu_feature_enabled(X86_FEATURE_IBPB_EXIT_TO_USER) &&
	    this_cpu_read(x86_ibpb_exit_to_user)) {
		indirect_branch_prediction_barrier();
		this_cpu_write(x86_ibpb_exit_to_user, false);
	}
}
#define arch_exit_to_user_mode_prepare arch_exit_to_user_mode_prepare

static __always_inline void arch_exit_to_user_mode(void)
{
	amd_clear_divider();
}
#define arch_exit_to_user_mode arch_exit_to_user_mode

#endif
