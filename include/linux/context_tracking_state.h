/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_CONTEXT_TRACKING_STATE_H
#define _LINUX_CONTEXT_TRACKING_STATE_H

#include <linux/percpu.h>
#include <linux/static_key.h>
#include <linux/context_tracking_irq.h>

/* Offset to allow distinguishing irq vs. task-based idle entry/exit. */
#define CT_NESTING_IRQ_NONIDLE	((LONG_MAX / 2) + 1)

enum ctx_state {
	CT_STATE_DISABLED	= -1,	/* returned by ct_state() if unknown */
	CT_STATE_KERNEL		= 0,
	CT_STATE_IDLE		= 1,
	CT_STATE_USER		= 2,
	CT_STATE_GUEST		= 3,
	CT_STATE_MAX		= 4,
};

struct context_tracking {
#ifdef CONFIG_CONTEXT_TRACKING
	atomic_t state;
#endif
#ifdef CONFIG_CONTEXT_TRACKING_IDLE
	long nesting;		/* Track process nesting level. */
	long nmi_nesting;	/* Track irq/NMI nesting level. */
#endif
};

/*
 * We cram two different things within the same atomic variable:
 *
 *                     CT_RCU_WATCHING_START  CT_STATE_START
 *                                |                |
 *                                v                v
 *     MSB [ RCU watching counter ][ context_state ] LSB
 *         ^                       ^
 *         |                       |
 * CT_RCU_WATCHING_END        CT_STATE_END
 *
 * Bits are used from the LSB upwards, so unused bits (if any) will always be in
 * upper bits of the variable.
 */
#ifdef CONFIG_CONTEXT_TRACKING
#define CT_SIZE (sizeof(((struct context_tracking *)0)->state) * BITS_PER_BYTE)

#define CT_STATE_WIDTH bits_per(CT_STATE_MAX - 1)
#define CT_STATE_START 0
#define CT_STATE_END   (CT_STATE_START + CT_STATE_WIDTH - 1)

#define CT_RCU_WATCHING_MAX_WIDTH (CT_SIZE - CT_STATE_WIDTH)
#define CT_RCU_WATCHING_WIDTH     (0 ? 2 : CT_RCU_WATCHING_MAX_WIDTH)
#define CT_RCU_WATCHING_START     (CT_STATE_END + 1)
#define CT_RCU_WATCHING_END       (CT_RCU_WATCHING_START + CT_RCU_WATCHING_WIDTH - 1)
#define CT_RCU_WATCHING           BIT(CT_RCU_WATCHING_START)

#define CT_STATE_MASK        GENMASK(CT_STATE_END,        CT_STATE_START)
#define CT_RCU_WATCHING_MASK GENMASK(CT_RCU_WATCHING_END, CT_RCU_WATCHING_START)

#define CT_UNUSED_WIDTH (CT_RCU_WATCHING_MAX_WIDTH - CT_RCU_WATCHING_WIDTH)

static_assert(CT_STATE_WIDTH        +
	      CT_RCU_WATCHING_WIDTH +
	      CT_UNUSED_WIDTH       ==
	      CT_SIZE);

DECLARE_PER_CPU(struct context_tracking, context_tracking);
#endif	/* CONFIG_CONTEXT_TRACKING */


#ifdef CONFIG_CONTEXT_TRACKING_IDLE
static __always_inline int ct_rcu_watching(void)
{
	return atomic_read(this_cpu_ptr(&context_tracking.state)) & CT_RCU_WATCHING_MASK;
}

static __always_inline int ct_rcu_watching_cpu(int cpu)
{
	struct context_tracking *ct = per_cpu_ptr(&context_tracking, cpu);

	return atomic_read(&ct->state) & CT_RCU_WATCHING_MASK;
}

static __always_inline int ct_rcu_watching_cpu_acquire(int cpu)
{
	struct context_tracking *ct = per_cpu_ptr(&context_tracking, cpu);

	return atomic_read_acquire(&ct->state) & CT_RCU_WATCHING_MASK;
}

static __always_inline long ct_nesting(void)
{
	return __this_cpu_read(context_tracking.nesting);
}

static __always_inline long ct_nesting_cpu(int cpu)
{
	struct context_tracking *ct = per_cpu_ptr(&context_tracking, cpu);

	return ct->nesting;
}

static __always_inline long ct_nmi_nesting(void)
{
	return __this_cpu_read(context_tracking.nmi_nesting);
}

static __always_inline long ct_nmi_nesting_cpu(int cpu)
{
	struct context_tracking *ct = per_cpu_ptr(&context_tracking, cpu);

	return ct->nmi_nesting;
}
#endif /* #ifdef CONFIG_CONTEXT_TRACKING_IDLE */

static __always_inline bool context_tracking_enabled(void) { return false; }
static __always_inline bool context_tracking_enabled_cpu(int cpu) { return false; }
static __always_inline bool context_tracking_enabled_this_cpu(void) { return false; }

#endif
