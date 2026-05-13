/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2025 Arm Ltd. */

#ifndef __ASM__MPAM_H
#define __ASM__MPAM_H

#include <linux/arm_mpam.h>
#include <linux/bitfield.h>
#include <linux/jump_label.h>
#include <linux/percpu.h>
#include <linux/sched.h>

#include <asm/sysreg.h>

DECLARE_STATIC_KEY_FALSE(mpam_enabled);
DECLARE_PER_CPU(u64, arm64_mpam_default);
DECLARE_PER_CPU(u64, arm64_mpam_current);

/*
 * The value of the MPAM0_EL1 sysreg when a task is in resctrl's default group.
 * This is used by the context switch code to use the resctrl CPU property
 * instead. The value is modified when CDP is enabled/disabled by mounting
 * the resctrl filesystem.
 */
extern u64 arm64_mpam_global_default;

static inline void mpam_thread_switch(struct task_struct *tsk) {}

#endif /* __ASM__MPAM_H */
