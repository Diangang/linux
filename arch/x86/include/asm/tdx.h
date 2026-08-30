/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2021-2022 Intel Corporation */
#ifndef _ASM_X86_TDX_H
#define _ASM_X86_TDX_H

#include <linux/init.h>
#include <linux/bits.h>
#include <linux/mmzone.h>

#include <asm/errno.h>
#include <asm/ptrace.h>
#include <asm/trapnr.h>
#include <asm/shared/tdx.h>

/*
 * SW-defined error codes.
 *
 * Bits 47:40 == 0xFF indicate Reserved status code class that never used by
 * TDX module.
 */
#define TDX_ERROR			_BITUL(63)
#define TDX_NON_RECOVERABLE		_BITUL(62)
#define TDX_SW_ERROR			(TDX_ERROR | GENMASK_ULL(47, 40))
#define TDX_SEAMCALL_VMFAILINVALID	(TDX_SW_ERROR | _UL(0xFFFF0000))

#define TDX_SEAMCALL_GP			(TDX_SW_ERROR | X86_TRAP_GP)
#define TDX_SEAMCALL_UD			(TDX_SW_ERROR | X86_TRAP_UD)

/*
 * TDX module SEAMCALL leaf function error codes
 */
#define TDX_SUCCESS		0ULL
#define TDX_RND_NO_ENTROPY	0x8000020300000000ULL

#ifndef __ASSEMBLER__

#include <uapi/asm/mce.h>
#include <asm/tdx_global_metadata.h>
#include <linux/pgtable.h>

/*
 * Used by the #VE exception handler to gather the #VE exception
 * info from the TDX module. This is a software only structure
 * and not part of the TDX module/VMM ABI.
 */
struct ve_info {
	u64 exit_reason;
	u64 exit_qual;
	/* Guest Linear (virtual) Address */
	u64 gla;
	/* Guest Physical Address */
	u64 gpa;
	u32 instr_len;
	u32 instr_info;
};


static inline void tdx_early_init(void) { };
static inline void tdx_halt(void) { };

static inline bool tdx_early_handle_ve(struct pt_regs *regs) { return false; }


static inline long tdx_kvm_hypercall(unsigned int nr, unsigned long p1,
				     unsigned long p2, unsigned long p3,
				     unsigned long p4)
{
	return -ENODEV;
}

static inline void tdx_init(void) { }
static inline u32 tdx_get_nr_guest_keyids(void) { return 0; }
static inline const char *tdx_dump_mce_info(struct mce *m) { return NULL; }
static inline const struct tdx_sys_info *tdx_get_sysinfo(void) { return NULL; }

static inline void tdx_cpu_flush_cache_for_kexec(void) { }

#endif /* !__ASSEMBLER__ */
#endif /* _ASM_X86_TDX_H */
