/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Clang Control Flow Integrity (CFI) support.
 *
 * Copyright (C) 2022 Google LLC
 */
#ifndef _LINUX_CFI_H
#define _LINUX_CFI_H

#include <linux/bug.h>
#include <linux/module.h>
#include <asm/cfi.h>


static inline int cfi_get_offset(void) { return 0; }
static inline u32 cfi_get_func_hash(void *func) { return 0; }

#define cfi_bpf_hash 0U
#define cfi_bpf_subprog_hash 0U


static inline bool is_cfi_trap(unsigned long addr) { return false; }

#ifndef CFI_NOSEAL
#define CFI_NOSEAL(x)
#endif

#endif /* _LINUX_CFI_H */
