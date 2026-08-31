// SPDX-License-Identifier: GPL-2.0-only
/*
 * Based on arch/arm/kernel/asm-offsets.c
 *
 * Copyright (C) 1995-2003 Russell King
 *               2001-2002 Keith Owens
 * Copyright (C) 2012 ARM Ltd.
 */
#define COMPILE_OFFSETS

#include <linux/arm_sdei.h>
#include <linux/sched.h>
#include <linux/ftrace.h>
#include <linux/mm.h>
#include <linux/suspend.h>
#include <asm/cpufeature.h>
#include <asm/fixmap.h>
#include <asm/thread_info.h>
#include <asm/memory.h>
#include <asm/smp_plat.h>
#include <linux/kbuild.h>
#include <linux/arm-smccc.h>

int main(void)
{
  DEFINE(TSK_TI_CPU,		offsetof(struct task_struct, thread_info.cpu));
  DEFINE(TSK_TI_FLAGS,		offsetof(struct task_struct, thread_info.flags));
  DEFINE(TSK_TI_PREEMPT,	offsetof(struct task_struct, thread_info.preempt_count));
  DEFINE(TSK_STACK,		offsetof(struct task_struct, stack));
#ifdef CONFIG_STACKPROTECTOR
  DEFINE(TSK_STACK_CANARY,	offsetof(struct task_struct, stack_canary));
#endif
  BLANK();
  DEFINE(THREAD_CPU_CONTEXT,	offsetof(struct task_struct, thread.cpu_context));
  DEFINE(THREAD_SCTLR_USER,	offsetof(struct task_struct, thread.sctlr_user));
#ifdef CONFIG_ARM64_PTR_AUTH
  DEFINE(THREAD_KEYS_USER,	offsetof(struct task_struct, thread.keys_user));
#endif
#ifdef CONFIG_ARM64_PTR_AUTH_KERNEL
  DEFINE(THREAD_KEYS_KERNEL,	offsetof(struct task_struct, thread.keys_kernel));
#endif
  BLANK();
  DEFINE(S_X0,			offsetof(struct pt_regs, regs[0]));
  DEFINE(S_X2,			offsetof(struct pt_regs, regs[2]));
  DEFINE(S_X4,			offsetof(struct pt_regs, regs[4]));
  DEFINE(S_X6,			offsetof(struct pt_regs, regs[6]));
  DEFINE(S_X8,			offsetof(struct pt_regs, regs[8]));
  DEFINE(S_X10,			offsetof(struct pt_regs, regs[10]));
  DEFINE(S_X12,			offsetof(struct pt_regs, regs[12]));
  DEFINE(S_X14,			offsetof(struct pt_regs, regs[14]));
  DEFINE(S_X16,			offsetof(struct pt_regs, regs[16]));
  DEFINE(S_X18,			offsetof(struct pt_regs, regs[18]));
  DEFINE(S_X20,			offsetof(struct pt_regs, regs[20]));
  DEFINE(S_X22,			offsetof(struct pt_regs, regs[22]));
  DEFINE(S_X24,			offsetof(struct pt_regs, regs[24]));
  DEFINE(S_X26,			offsetof(struct pt_regs, regs[26]));
  DEFINE(S_X28,			offsetof(struct pt_regs, regs[28]));
  DEFINE(S_FP,			offsetof(struct pt_regs, regs[29]));
  DEFINE(S_LR,			offsetof(struct pt_regs, regs[30]));
  DEFINE(S_SP,			offsetof(struct pt_regs, sp));
  DEFINE(S_PC,			offsetof(struct pt_regs, pc));
  DEFINE(S_PSTATE,		offsetof(struct pt_regs, pstate));
  DEFINE(S_SYSCALLNO,		offsetof(struct pt_regs, syscallno));
  DEFINE(S_SDEI_TTBR1,		offsetof(struct pt_regs, sdei_ttbr1));
  DEFINE(S_PMR,			offsetof(struct pt_regs, pmr));
  DEFINE(S_STACKFRAME,		offsetof(struct pt_regs, stackframe));
  DEFINE(S_STACKFRAME_TYPE,	offsetof(struct pt_regs, stackframe.type));
  DEFINE(PT_REGS_SIZE,		sizeof(struct pt_regs));
  BLANK();
  DEFINE(CPU_BOOT_TASK,		offsetof(struct secondary_data, task));
  BLANK();
  DEFINE(FTR_OVR_VAL_OFFSET,	offsetof(struct arm64_ftr_override, val));
  DEFINE(FTR_OVR_MASK_OFFSET,	offsetof(struct arm64_ftr_override, mask));
  BLANK();
  DEFINE(ARM_SMCCC_RES_X0_OFFS,		offsetof(struct arm_smccc_res, a0));
  DEFINE(ARM_SMCCC_RES_X2_OFFS,		offsetof(struct arm_smccc_res, a2));
  DEFINE(ARM_SMCCC_QUIRK_ID_OFFS,	offsetof(struct arm_smccc_quirk, id));
  DEFINE(ARM_SMCCC_QUIRK_STATE_OFFS,	offsetof(struct arm_smccc_quirk, state));
  DEFINE(ARM_SMCCC_1_2_REGS_X0_OFFS,	offsetof(struct arm_smccc_1_2_regs, a0));
  DEFINE(ARM_SMCCC_1_2_REGS_X2_OFFS,	offsetof(struct arm_smccc_1_2_regs, a2));
  DEFINE(ARM_SMCCC_1_2_REGS_X4_OFFS,	offsetof(struct arm_smccc_1_2_regs, a4));
  DEFINE(ARM_SMCCC_1_2_REGS_X6_OFFS,	offsetof(struct arm_smccc_1_2_regs, a6));
  DEFINE(ARM_SMCCC_1_2_REGS_X8_OFFS,	offsetof(struct arm_smccc_1_2_regs, a8));
  DEFINE(ARM_SMCCC_1_2_REGS_X10_OFFS,	offsetof(struct arm_smccc_1_2_regs, a10));
  DEFINE(ARM_SMCCC_1_2_REGS_X12_OFFS,	offsetof(struct arm_smccc_1_2_regs, a12));
  DEFINE(ARM_SMCCC_1_2_REGS_X14_OFFS,	offsetof(struct arm_smccc_1_2_regs, a14));
  DEFINE(ARM_SMCCC_1_2_REGS_X16_OFFS,	offsetof(struct arm_smccc_1_2_regs, a16));
  BLANK();
  DEFINE(ARM64_FTR_SYSVAL,	offsetof(struct arm64_ftr_reg, sys_val));
  BLANK();
#ifdef CONFIG_UNMAP_KERNEL_AT_EL0
  DEFINE(TRAMP_VALIAS,		TRAMP_VALIAS);
#endif
#ifdef CONFIG_ARM64_PTR_AUTH
  DEFINE(PTRAUTH_USER_KEY_APIA,		offsetof(struct ptrauth_keys_user, apia));
#ifdef CONFIG_ARM64_PTR_AUTH_KERNEL
  DEFINE(PTRAUTH_KERNEL_KEY_APIA,	offsetof(struct ptrauth_keys_kernel, apia));
#endif
  BLANK();
#endif
  BLANK();
  DEFINE(PIE_E0_ASM, PIE_E0);
  DEFINE(PIE_E1_ASM, PIE_E1);
  return 0;
}
