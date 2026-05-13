/* SPDX-License-Identifier: GPL-2.0 */

#ifndef _ASM_VERMAGIC_H
#define _ASM_VERMAGIC_H

#ifdef CONFIG_X86_64
/* X86_64 does not define MODULE_PROC_FAMILY */
#elif 0
#define MODULE_PROC_FAMILY "586 "
#elif 0
#define MODULE_PROC_FAMILY "586TSC "
#elif 0
#define MODULE_PROC_FAMILY "586MMX "
#elif 0
#define MODULE_PROC_FAMILY "ATOM "
#elif 0
#define MODULE_PROC_FAMILY "686 "
#elif 0
#define MODULE_PROC_FAMILY "CRUSOE "
#elif 0
#define MODULE_PROC_FAMILY "EFFICEON "
#elif 0
#define MODULE_PROC_FAMILY "CYRIXIII "
#else
#error unknown processor family
#endif

# define MODULE_ARCH_VERMAGIC ""

#endif /* _ASM_VERMAGIC_H */
