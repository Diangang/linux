/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_X86_PARAVIRT_H
#define _ASM_X86_PARAVIRT_H
/* Various instructions on x86 need to be replaced for
 * para-virtualization: those hooks are defined here. */

#ifndef __ASSEMBLER__
#include <asm/paravirt-base.h>
#endif
#include <asm/paravirt_types.h>

#ifdef CONFIG_PARAVIRT
#include <asm/pgtable_types.h>
#include <asm/asm.h>
#include <asm/nospec-branch.h>

#ifndef __ASSEMBLER__
#include <linux/types.h>
#include <linux/cpumask.h>
#include <asm/frame.h>

void native_flush_tlb_local(void);
void native_flush_tlb_global(void);
void native_flush_tlb_one_user(unsigned long addr);
void native_flush_tlb_multi(const struct cpumask *cpumask,
			     const struct flush_tlb_info *info);

static inline void __flush_tlb_local(void)
{
	PVOP_VCALL0(pv_ops, mmu.flush_tlb_user);
}

static inline void __flush_tlb_global(void)
{
	PVOP_VCALL0(pv_ops, mmu.flush_tlb_kernel);
}

static inline void __flush_tlb_one_user(unsigned long addr)
{
	PVOP_VCALL1(pv_ops, mmu.flush_tlb_one_user, addr);
}

static inline void __flush_tlb_multi(const struct cpumask *cpumask,
				      const struct flush_tlb_info *info)
{
	PVOP_VCALL2(pv_ops, mmu.flush_tlb_multi, cpumask, info);
}

static inline void paravirt_arch_exit_mmap(struct mm_struct *mm)
{
	PVOP_VCALL1(pv_ops, mmu.exit_mmap, mm);
}

static inline void notify_page_enc_status_changed(unsigned long pfn,
						  int npages, bool enc)
{
	PVOP_VCALL3(pv_ops, mmu.notify_page_enc_status_changed, pfn, npages, enc);
}

static __always_inline void arch_safe_halt(void)
{
	PVOP_VCALL0(pv_ops, irq.safe_halt);
}

static inline void halt(void)
{
	PVOP_VCALL0(pv_ops, irq.halt);
}


#else  /* __ASSEMBLER__ */

#ifdef CONFIG_X86_64
#endif	/* CONFIG_X86_64 */

#endif /* __ASSEMBLER__ */
#else  /* CONFIG_PARAVIRT */
# define default_banner x86_init_noop
#endif /* !CONFIG_PARAVIRT */

#ifndef __ASSEMBLER__
static inline void paravirt_enter_mmap(struct mm_struct *mm)
{
}

#ifndef CONFIG_PARAVIRT
static inline void paravirt_arch_exit_mmap(struct mm_struct *mm)
{
}
#endif

#endif /* __ASSEMBLER__ */
#endif /* _ASM_X86_PARAVIRT_H */
