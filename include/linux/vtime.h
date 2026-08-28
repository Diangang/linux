/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_KERNEL_VTIME_H
#define _LINUX_KERNEL_VTIME_H

#include <linux/context_tracking_state.h>
#include <linux/sched.h>

/*
 * Common vtime APIs
 */

static inline void vtime_user_enter(struct task_struct *tsk) { }
static inline void vtime_user_exit(struct task_struct *tsk) { }
static inline void vtime_guest_enter(struct task_struct *tsk) { }
static inline void vtime_guest_exit(struct task_struct *tsk) { }
static inline void vtime_init_idle(struct task_struct *tsk, int cpu) { }

static inline void vtime_account_irq(struct task_struct *tsk, unsigned int offset) { }
static inline void vtime_account_softirq(struct task_struct *tsk) { }
static inline void vtime_account_hardirq(struct task_struct *tsk) { }
static inline void vtime_flush(struct task_struct *tsk) { }

static inline bool vtime_accounting_enabled_this_cpu(void) { return false; }
static inline void vtime_task_switch(struct task_struct *prev) { }

static __always_inline void vtime_account_guest_enter(void)
{
	current->flags |= PF_VCPU;
}

static __always_inline void vtime_account_guest_exit(void)
{
	current->flags &= ~PF_VCPU;
}
static inline void irqtime_account_irq(struct task_struct *tsk, unsigned int offset) { }

static inline void account_softirq_enter(struct task_struct *tsk)
{
	vtime_account_irq(tsk, SOFTIRQ_OFFSET);
	irqtime_account_irq(tsk, SOFTIRQ_OFFSET);
}

static inline void account_softirq_exit(struct task_struct *tsk)
{
	vtime_account_softirq(tsk);
	irqtime_account_irq(tsk, 0);
}

static inline void account_hardirq_enter(struct task_struct *tsk)
{
	vtime_account_irq(tsk, HARDIRQ_OFFSET);
	irqtime_account_irq(tsk, HARDIRQ_OFFSET);
}

static inline void account_hardirq_exit(struct task_struct *tsk)
{
	vtime_account_hardirq(tsk);
	irqtime_account_irq(tsk, 0);
}

#endif /* _LINUX_KERNEL_VTIME_H */
