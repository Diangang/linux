/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_X86_RESCTRL_H
#define _ASM_X86_RESCTRL_H


static inline void resctrl_arch_sched_in(struct task_struct *tsk) {}
static inline void resctrl_cpu_detect(struct cpuinfo_x86 *c) {}


#endif /* _ASM_X86_RESCTRL_H */
