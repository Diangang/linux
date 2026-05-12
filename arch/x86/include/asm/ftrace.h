/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_X86_FTRACE_H
#define _ASM_X86_FTRACE_H

#include <asm/ptrace.h>

#ifndef __ASSEMBLER__

void prepare_ftrace_return(unsigned long ip, unsigned long *parent,
			   unsigned long frame_pointer);

static inline void set_ftrace_ops_ro(void) { }

#define ARCH_HAS_SYSCALL_MATCH_SYM_NAME
static inline bool arch_syscall_match_sym_name(const char *sym, const char *name)
{
	/*
	 * Compare the symbol name with the system call name. Skip the
	 * "__x64_sys", "__ia32_sys", "__do_sys" or simple "sys" prefix.
	 */
	return !strcmp(sym + 3, name + 3) ||
		(!strncmp(sym, "__x64_", 6) && !strcmp(sym + 9, name + 3)) ||
		(!strncmp(sym, "__ia32_", 7) && !strcmp(sym + 10, name + 3)) ||
		(!strncmp(sym, "__do_sys", 8) && !strcmp(sym + 8, name + 3));
}

#ifndef COMPILE_OFFSETS

#if defined(CONFIG_FTRACE_SYSCALLS) && defined(CONFIG_IA32_EMULATION)
#include <linux/compat.h>

/*
 * Because ia32 syscalls do not map to x86_64 syscall numbers
 * this screws up the trace output when tracing a ia32 task.
 * Instead of reporting bogus syscalls, just do not trace them.
 *
 * If the user really wants these, then they should use the
 * raw syscall tracepoints with filtering.
 */
#define ARCH_TRACE_IGNORE_COMPAT_SYSCALLS 1
static inline bool arch_trace_is_compat_syscall(struct pt_regs *regs)
{
	return in_32bit_syscall();
}
#endif /* CONFIG_FTRACE_SYSCALLS && CONFIG_IA32_EMULATION */
#endif /* !COMPILE_OFFSETS */
#endif /* !__ASSEMBLER__ */

#endif /* _ASM_X86_FTRACE_H */
