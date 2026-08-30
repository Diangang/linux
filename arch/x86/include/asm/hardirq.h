/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_X86_HARDIRQ_H
#define _ASM_X86_HARDIRQ_H

#include <linux/threads.h>

typedef struct {
	unsigned int __nmi_count;	/* arch dependent */
#ifdef CONFIG_X86_LOCAL_APIC
	unsigned int apic_timer_irqs;	/* arch dependent */
	unsigned int irq_spurious_count;
	unsigned int icr_read_retry_count;
#endif
	unsigned int x86_platform_ipis;	/* arch dependent */
	unsigned int apic_perf_irqs;
	unsigned int apic_irq_work_irqs;
#ifdef CONFIG_SMP
	unsigned int irq_resched_count;
	unsigned int irq_call_count;
#endif
	unsigned int irq_tlb_count;
#ifdef CONFIG_X86_MCE_THRESHOLD
	unsigned int irq_threshold_count;
#endif
#ifdef CONFIG_X86_MCE_AMD
	unsigned int irq_deferred_error_count;
#endif
} ____cacheline_aligned irq_cpustat_t;

DECLARE_PER_CPU_SHARED_ALIGNED(irq_cpustat_t, irq_stat);

#define __ARCH_IRQ_STAT

#define inc_irq_stat(member)	this_cpu_inc(irq_stat.member)

extern void ack_bad_irq(unsigned int irq);

extern u64 arch_irq_stat_cpu(unsigned int cpu);
#define arch_irq_stat_cpu	arch_irq_stat_cpu

extern u64 arch_irq_stat(void);
#define arch_irq_stat		arch_irq_stat

DECLARE_PER_CPU_CACHE_HOT(u16, __softirq_pending);
#define local_softirq_pending_ref       __softirq_pending

static __always_inline void kvm_set_cpu_l1tf_flush_l1d(void) { }

#endif /* _ASM_X86_HARDIRQ_H */
