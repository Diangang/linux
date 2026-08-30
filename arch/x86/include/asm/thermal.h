/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_X86_THERMAL_H
#define _ASM_X86_THERMAL_H

static inline void therm_lvt_init(void)				{ }
static inline void intel_init_thermal(struct cpuinfo_x86 *c)	{ }

#endif /* _ASM_X86_THERMAL_H */
