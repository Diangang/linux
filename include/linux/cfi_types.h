/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Clang Control Flow Integrity (CFI) type definitions.
 */
#ifndef _LINUX_CFI_TYPES_H
#define _LINUX_CFI_TYPES_H

#ifdef __ASSEMBLY__
#include <linux/linkage.h>


#define SYM_TYPED_START(name, linkage, align...)	\
	SYM_START(name, linkage, align)


#ifndef SYM_TYPED_FUNC_START
#define SYM_TYPED_FUNC_START(name) 			\
	SYM_TYPED_START(name, SYM_L_GLOBAL, SYM_A_ALIGN)
#endif

#else /* __ASSEMBLY__ */


#endif /* __ASSEMBLY__ */
#endif /* _LINUX_CFI_TYPES_H */
