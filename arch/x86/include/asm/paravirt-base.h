/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef _ASM_X86_PARAVIRT_BASE_H
#define _ASM_X86_PARAVIRT_BASE_H

/*
 * Wrapper type for pointers to code which uses the non-standard
 * calling convention.  See PV_CALL_SAVE_REGS_THUNK below.
 */
struct paravirt_callee_save {
	void *func;
};

struct pv_info {
	bool io_delay;

	const char *name;
};

void default_banner(void);
extern struct pv_info pv_info;
unsigned long paravirt_ret0(void);
#define paravirt_nop	((void *)nop_func)

#ifdef CONFIG_PARAVIRT
#define call_io_delay() pv_info.io_delay
#endif

static inline void paravirt_set_cap(void) { }

#endif /* _ASM_X86_PARAVIRT_BASE_H */
