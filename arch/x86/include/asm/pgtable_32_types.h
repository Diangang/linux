/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_X86_PGTABLE_32_TYPES_H
#define _ASM_X86_PGTABLE_32_TYPES_H

/*
 * The Linux x86 paging architecture is 'compile-time dual-mode', it
 * implements both the traditional 2-level x86 page tables and the
 * newer 3-level PAE-mode page tables.
 */
# include <asm/pgtable-2level_types.h>

#define pgtable_l5_enabled() 0

#define PGDIR_SIZE	(1UL << PGDIR_SHIFT)
#define PGDIR_MASK	(~(PGDIR_SIZE - 1))

#endif /* _ASM_X86_PGTABLE_32_TYPES_H */
