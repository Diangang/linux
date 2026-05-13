/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_X86_DIV64_H
#define _ASM_X86_DIV64_H

# include <asm-generic/div64.h>

/*
 * Will generate an #DE when the result doesn't fit u64, could fix with an
 * __ex_table[] entry when it becomes an issue.
 */
static inline u64 mul_u64_add_u64_div_u64(u64 rax, u64 mul, u64 add, u64 div)
{
	u64 rdx;

	asm ("mulq %[mul]" : "+a" (rax), "=d" (rdx) : [mul] "rm" (mul));

	if (!statically_true(!add))
		asm ("addq %[add], %[lo]; adcq $0, %[hi]" :
			[lo] "+r" (rax), [hi] "+r" (rdx) : [add] "irm" (add));

	asm ("divq %[div]" : "+a" (rax), "+d" (rdx) : [div] "rm" (div));

	return rax;
}
#define mul_u64_add_u64_div_u64 mul_u64_add_u64_div_u64

static inline u64 mul_u64_u32_div(u64 a, u32 mul, u32 div)
{
	return mul_u64_add_u64_div_u64(a, mul, 0, div);
}
#define mul_u64_u32_div	mul_u64_u32_div


#endif /* _ASM_X86_DIV64_H */
