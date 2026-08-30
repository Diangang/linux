/* SPDX-License-Identifier: GPL-2.0 */

/*
 * Jailhouse paravirt detection
 *
 * Copyright (c) Siemens AG, 2015-2017
 *
 * Authors:
 *  Jan Kiszka <jan.kiszka@siemens.com>
 */

#ifndef _ASM_X86_JAILHOUSE_PARA_H
#define _ASM_X86_JAILHOUSE_PARA_H

#include <linux/types.h>

static inline bool jailhouse_paravirt(void)
{
	return false;
}

#endif /* _ASM_X86_JAILHOUSE_PARA_H */
