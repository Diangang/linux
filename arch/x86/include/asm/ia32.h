/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_X86_IA32_H
#define _ASM_X86_IA32_H


static __always_inline bool ia32_enabled(void)
{
	return 0;
}

static inline void ia32_disable(void) {}


static inline bool ia32_enabled_verbose(void)
{
	return ia32_enabled();
}

#endif /* _ASM_X86_IA32_H */
