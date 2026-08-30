/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __ASM_LSUI_H
#define __ASM_LSUI_H

#include <linux/compiler_types.h>
#include <linux/stringify.h>
#include <asm/alternative.h>
#include <asm/alternative-macros.h>
#include <asm/cpucaps.h>

#define __LSUI_PREAMBLE	".arch_extension lsui\n"


#define __lsui_llsc_body(op, ...)	__llsc_##op(__VA_ARGS__)


#endif	/* __ASM_LSUI_H */
