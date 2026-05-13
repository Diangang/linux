// SPDX-License-Identifier: GPL-2.0
/*
 * Clang Control Flow Integrity (CFI) error handling.
 *
 * Copyright (C) 2022 Google LLC
 */

#include <linux/bpf.h>
#include <linux/cfi_types.h>
#include <linux/cfi.h>

bool cfi_warn __ro_after_init = IS_ENABLED(CONFIG_CFI_PERMISSIVE);

enum bug_trap_type report_cfi_failure(struct pt_regs *regs, unsigned long addr,
				      unsigned long *target, u32 type)
{
	if (target)
		pr_err("CFI failure at %pS (target: %pS; expected type: 0x%08x)\n",
		       (void *)addr, (void *)*target, type);
	else
		pr_err("CFI failure at %pS (no target information)\n",
		       (void *)addr);

	if (cfi_warn) {
		__warn(NULL, 0, (void *)addr, 0, regs, NULL);
		return BUG_TRAP_TYPE_WARN;
	}

	return BUG_TRAP_TYPE_BUG;
}

/*
 * Declare two non-existent functions with types that match bpf_func_t and
 * bpf_callback_t pointers, and use DEFINE_CFI_TYPE to define type hash
 * variables for each function type. The cfi_bpf_* variables are used by
 * arch-specific BPF JIT implementations to ensure indirectly callable JIT
 * code has matching CFI type hashes.
 */
extern typeof(*(bpf_func_t)0) __bpf_prog_runX;
DEFINE_CFI_TYPE(cfi_bpf_hash, __bpf_prog_runX);

extern typeof(*(bpf_callback_t)0) __bpf_callback_fn;
DEFINE_CFI_TYPE(cfi_bpf_subprog_hash, __bpf_callback_fn);
