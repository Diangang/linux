/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __ASM_CPUIDLE_H
#define __ASM_CPUIDLE_H

#include <asm/proc-fns.h>

struct arm_cpuidle_irq_context { };

#define arm_cpuidle_save_irq_context(c)		(void)c
#define arm_cpuidle_restore_irq_context(c)	(void)c
#endif
