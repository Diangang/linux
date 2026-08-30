/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_X86_SECCOMP_H
#define _ASM_X86_SECCOMP_H

#include <asm/unistd.h>



#ifdef CONFIG_X86_64
# define SECCOMP_ARCH_NATIVE		AUDIT_ARCH_X86_64
# define SECCOMP_ARCH_NATIVE_NR		NR_syscalls
# define SECCOMP_ARCH_NATIVE_NAME	"x86_64"
/*
 * x32 will have __X32_SYSCALL_BIT set in syscall number. We don't support
 * caching them and they are treated as out of range syscalls, which will
 * always pass through the BPF filter.
 */
#else /* !CONFIG_X86_64 */
# define SECCOMP_ARCH_NATIVE		AUDIT_ARCH_I386
# define SECCOMP_ARCH_NATIVE_NR	        NR_syscalls
# define SECCOMP_ARCH_NATIVE_NAME	"ia32"
#endif

#include <asm-generic/seccomp.h>

#endif /* _ASM_X86_SECCOMP_H */
