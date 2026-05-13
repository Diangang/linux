// SPDX-License-Identifier: GPL-2.0
#ifndef __LINUX_KBUILD_H
# error "Please do not build this file directly, build asm-offsets.c instead"
#endif

#include <asm/ia32.h>


int main(void)
{
#ifdef CONFIG_PARAVIRT
	BLANK();
#endif


#define ENTRY(entry) OFFSET(pt_regs_ ## entry, pt_regs, entry)
	ENTRY(bx);
	ENTRY(cx);
	ENTRY(dx);
	ENTRY(sp);
	ENTRY(bp);
	ENTRY(si);
	ENTRY(di);
	ENTRY(r8);
	ENTRY(r9);
	ENTRY(r10);
	ENTRY(r11);
	ENTRY(r12);
	ENTRY(r13);
	ENTRY(r14);
	ENTRY(r15);
	ENTRY(flags);
	BLANK();
#undef ENTRY

#define ENTRY(entry) OFFSET(saved_context_ ## entry, saved_context, entry)
	ENTRY(cr0);
	ENTRY(cr2);
	ENTRY(cr3);
	ENTRY(cr4);
	ENTRY(gdt_desc);
	BLANK();
#undef ENTRY

	return 0;
}
