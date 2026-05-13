/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_X86_DOUBLEFAULT_H
#define _ASM_X86_DOUBLEFAULT_H

#include <linux/linkage.h>

static inline void doublefault_init_cpu_tss(void)
{
}

asmlinkage void __noreturn doublefault_shim(void);

#endif /* _ASM_X86_DOUBLEFAULT_H */
