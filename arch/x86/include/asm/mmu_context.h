/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_X86_MMU_CONTEXT_H
#define _ASM_X86_MMU_CONTEXT_H

#include <linux/atomic.h>
#include <linux/mm_types.h>
#include <linux/pkeys.h>

#include <asm/tlbflush.h>
#include <asm/debugreg.h>
#include <asm/gsseg.h>
#include <asm/desc.h>

extern atomic64_t last_mm_ctx_id;


static inline unsigned long mm_lam_cr3_mask(struct mm_struct *mm)
{
	return 0;
}

static inline void dup_lam(struct mm_struct *oldmm, struct mm_struct *mm)
{
}

static inline void mm_reset_untag_mask(struct mm_struct *mm)
{
}

extern void mm_init_global_asid(struct mm_struct *mm);
extern void mm_free_global_asid(struct mm_struct *mm);

/*
 * Init a new mm.  Used on mm copies, like at fork()
 * and on mm's that are brand-new, like at execve().
 */
#define init_new_context init_new_context
static inline int init_new_context(struct task_struct *tsk,
				   struct mm_struct *mm)
{
	mutex_init(&mm->context.lock);

	mm->context.ctx_id = atomic64_inc_return(&last_mm_ctx_id);
	atomic64_set(&mm->context.tlb_gen, 0);
	mm->context.next_trim_cpumask = jiffies + HZ;

#ifdef CONFIG_X86_INTEL_MEMORY_PROTECTION_KEYS
	if (cpu_feature_enabled(X86_FEATURE_OSPKE)) {
		/* pkey 0 is the default and allocated implicitly */
		mm->context.pkey_allocation_map = 0x1;
		/* -1 means unallocated or invalid */
		mm->context.execute_only_pkey = -1;
	}
#endif

	mm_init_global_asid(mm);
	mm_reset_untag_mask(mm);
	return 0;
}

#define destroy_context destroy_context
static inline void destroy_context(struct mm_struct *mm)
{
	mm_free_global_asid(mm);
}

extern void switch_mm(struct mm_struct *prev, struct mm_struct *next,
		      struct task_struct *tsk);

extern void switch_mm_irqs_off(struct mm_struct *prev, struct mm_struct *next,
			       struct task_struct *tsk);
#define switch_mm_irqs_off switch_mm_irqs_off

#define activate_mm(prev, next)			\
do {						\
	switch_mm_irqs_off((prev), (next), NULL);	\
} while (0);

#define deactivate_mm(tsk, mm)			\
do {						\
	shstk_free(tsk);			\
	load_gs_index(0);			\
	loadsegment(fs, 0);			\
} while (0)

static inline void arch_dup_pkeys(struct mm_struct *oldmm,
				  struct mm_struct *mm)
{
#ifdef CONFIG_X86_INTEL_MEMORY_PROTECTION_KEYS
	if (!cpu_feature_enabled(X86_FEATURE_OSPKE))
		return;

	/* Duplicate the oldmm pkey state in mm: */
	mm->context.pkey_allocation_map = oldmm->context.pkey_allocation_map;
	mm->context.execute_only_pkey   = oldmm->context.execute_only_pkey;
#endif
}

static inline int arch_dup_mmap(struct mm_struct *oldmm, struct mm_struct *mm)
{
	arch_dup_pkeys(oldmm, mm);
	dup_lam(oldmm, mm);
	return 0;
}

static inline void arch_exit_mmap(struct mm_struct *mm)
{
}

#ifdef CONFIG_X86_64
static inline bool is_64bit_mm(struct mm_struct *mm)
{
	return true;
}
#else
static inline bool is_64bit_mm(struct mm_struct *mm)
{
	return false;
}
#endif

static inline bool is_notrack_mm(struct mm_struct *mm)
{
	return test_bit(MM_CONTEXT_NOTRACK, &mm->context.flags);
}

static inline void set_notrack_mm(struct mm_struct *mm)
{
	set_bit(MM_CONTEXT_NOTRACK, &mm->context.flags);
}

/*
 * We only want to enforce protection keys on the current process
 * because we effectively have no access to PKRU for other
 * processes or any way to tell *which * PKRU in a threaded
 * process we could use.
 *
 * So do not enforce things if the VMA is not from the current
 * mm, or if we are in a kernel thread.
 */
static inline bool arch_vma_access_permitted(struct vm_area_struct *vma,
		bool write, bool execute, bool foreign)
{
	/* pkeys never affect instruction fetches */
	if (execute)
		return true;
	/* allow access if the VMA is not one from this process */
	if (foreign || vma_is_foreign(vma))
		return true;
	return __pkru_allows_pkey(vma_pkey(vma), write);
}

unsigned long __get_current_cr3_fast(void);

#include <asm-generic/mmu_context.h>

extern struct mm_struct *use_temporary_mm(struct mm_struct *temp_mm);
extern void unuse_temporary_mm(struct mm_struct *prev_mm);

#endif /* _ASM_X86_MMU_CONTEXT_H */
