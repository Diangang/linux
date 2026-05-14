/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_GENERIC_KPROBES_H
#define _ASM_GENERIC_KPROBES_H

#if defined(__KERNEL__) && !defined(__ASSEMBLY__)
# define NOKPROBE_SYMBOL(fname)
# define __kprobes
# define nokprobe_inline	inline
#endif /* defined(__KERNEL__) && !defined(__ASSEMBLY__) */

#endif /* _ASM_GENERIC_KPROBES_H */
