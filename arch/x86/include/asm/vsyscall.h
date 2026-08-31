/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_X86_VSYSCALL_H
#define _ASM_X86_VSYSCALL_H

#include <uapi/asm/vsyscall.h>
#include <asm/page_types.h>

/*
 * The legacy vsyscall address remains reserved in the kernel portion of the
 * address space.
 */
static inline bool is_vsyscall_vaddr(unsigned long vaddr)
{
	return unlikely((vaddr & PAGE_MASK) == VSYSCALL_ADDR);
}

#endif /* _ASM_X86_VSYSCALL_H */
