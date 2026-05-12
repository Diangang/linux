// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (2004) Linus Torvalds
 *
 * Author: Zwane Mwaikambo <zwane@fsmlabs.com>
 *
 * Copyright (2004, 2005) Ingo Molnar
 *
 * This file contains the spinlock/rwlock implementations for the
 * SMP and the DEBUG_SPINLOCK cases. (UP-nondebug inlines them)
 *
 * Note that some architectures have special knowledge about the
 * stack frames of these functions in their profile_pc. If you
 * change anything significant here that could change the stack
 * frame contact the architecture maintainers.
 */

#include <linux/linkage.h>
#include <linux/preempt.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/debug_locks.h>
#include <linux/export.h>

#ifdef CONFIG_MMIOWB
#ifndef arch_mmiowb_state
DEFINE_PER_CPU(struct mmiowb_state, __mmiowb_state);
EXPORT_PER_CPU_SYMBOL(__mmiowb_state);
#endif
#endif

/*
 * If lockdep is enabled then we use the non-preemption spin-ops
 * even on CONFIG_PREEMPT, because lockdep assumes that interrupts are
 * not re-enabled during lock-acquire (which the preempt-spin-ops do):
 */
#if !defined(CONFIG_GENERIC_LOCKBREAK) || defined(CONFIG_DEBUG_LOCK_ALLOC)
/*
 * The __lock_function inlines are taken from
 * spinlock : include/linux/spinlock_api_smp.h
 * rwlock   : include/linux/rwlock_api_smp.h
 */
#else

/*
 * Some architectures can relax in favour of the CPU owning the lock.
 */
#ifndef arch_read_relax
# define arch_read_relax(l)	cpu_relax()
#endif
#ifndef arch_write_relax
# define arch_write_relax(l)	cpu_relax()
#endif
#ifndef arch_spin_relax
# define arch_spin_relax(l)	cpu_relax()
#endif

/*
 * We build the __lock_function inlines here. They are too large for
 * inlining all over the place, but here is only one user per function
 * which embeds them into the calling _lock_function below.
 *
 * This could be a long-held lock. We both prepare to spin for a long
 * time (making _this_ CPU preemptible if possible), and we also signal
 * towards that other CPU that it should break the lock ASAP.
 */
#define BUILD_LOCK_OPS(op, locktype, lock_ctx_op)			\
static void __lockfunc __raw_##op##_lock(locktype##_t *lock)		\
	lock_ctx_op(lock)						\
{									\
	for (;;) {							\
		preempt_disable();					\
		if (likely(do_raw_##op##_trylock(lock)))		\
			break;						\
		preempt_enable();					\
									\
		arch_##op##_relax(&lock->raw_lock);			\
	}								\
}									\
									\
static unsigned long __lockfunc __raw_##op##_lock_irqsave(locktype##_t *lock) \
	lock_ctx_op(lock)						\
{									\
	unsigned long flags;						\
									\
	for (;;) {							\
		preempt_disable();					\
		local_irq_save(flags);					\
		if (likely(do_raw_##op##_trylock(lock)))		\
			break;						\
		local_irq_restore(flags);				\
		preempt_enable();					\
									\
		arch_##op##_relax(&lock->raw_lock);			\
	}								\
									\
	return flags;							\
}									\
									\
static void __lockfunc __raw_##op##_lock_irq(locktype##_t *lock)	\
	lock_ctx_op(lock)						\
{									\
	_raw_##op##_lock_irqsave(lock);					\
}									\
									\
static void __lockfunc __raw_##op##_lock_bh(locktype##_t *lock)		\
	lock_ctx_op(lock)						\
{									\
	unsigned long flags;						\
									\
	/*							*/	\
	/* Careful: we must exclude softirqs too, hence the	*/	\
	/* irq-disabling. We use the generic preemption-aware	*/	\
	/* function:						*/	\
	/**/								\
	flags = _raw_##op##_lock_irqsave(lock);				\
	local_bh_disable();						\
	local_irq_restore(flags);					\
}									\

/*
 * Build preemption-friendly versions of the following
 * lock-spinning functions:
 *
 *         __[spin|read|write]_lock()
 *         __[spin|read|write]_lock_irq()
 *         __[spin|read|write]_lock_irqsave()
 *         __[spin|read|write]_lock_bh()
 */
BUILD_LOCK_OPS(spin, raw_spinlock, __acquires);

#ifndef CONFIG_PREEMPT_RT
BUILD_LOCK_OPS(read, rwlock, __acquires_shared);
BUILD_LOCK_OPS(write, rwlock, __acquires);
#endif

#endif

noinline int __lockfunc _raw_spin_trylock(raw_spinlock_t *lock)
{
	return __raw_spin_trylock(lock);
}
EXPORT_SYMBOL(_raw_spin_trylock);

noinline int __lockfunc _raw_spin_trylock_bh(raw_spinlock_t *lock)
{
	return __raw_spin_trylock_bh(lock);
}
EXPORT_SYMBOL(_raw_spin_trylock_bh);

noinline void __lockfunc _raw_spin_lock(raw_spinlock_t *lock)
{
	__raw_spin_lock(lock);
}
EXPORT_SYMBOL(_raw_spin_lock);

noinline unsigned long __lockfunc _raw_spin_lock_irqsave(raw_spinlock_t *lock)
{
	return __raw_spin_lock_irqsave(lock);
}
EXPORT_SYMBOL(_raw_spin_lock_irqsave);

noinline void __lockfunc _raw_spin_lock_irq(raw_spinlock_t *lock)
{
	__raw_spin_lock_irq(lock);
}
EXPORT_SYMBOL(_raw_spin_lock_irq);

noinline void __lockfunc _raw_spin_lock_bh(raw_spinlock_t *lock)
{
	__raw_spin_lock_bh(lock);
}
EXPORT_SYMBOL(_raw_spin_lock_bh);

#ifdef CONFIG_UNINLINE_SPIN_UNLOCK
noinline void __lockfunc _raw_spin_unlock(raw_spinlock_t *lock)
{
	__raw_spin_unlock(lock);
}
EXPORT_SYMBOL(_raw_spin_unlock);
#endif

noinline void __lockfunc _raw_spin_unlock_irqrestore(raw_spinlock_t *lock, unsigned long flags)
{
	__raw_spin_unlock_irqrestore(lock, flags);
}
EXPORT_SYMBOL(_raw_spin_unlock_irqrestore);

noinline void __lockfunc _raw_spin_unlock_irq(raw_spinlock_t *lock)
{
	__raw_spin_unlock_irq(lock);
}
EXPORT_SYMBOL(_raw_spin_unlock_irq);

noinline void __lockfunc _raw_spin_unlock_bh(raw_spinlock_t *lock)
{
	__raw_spin_unlock_bh(lock);
}
EXPORT_SYMBOL(_raw_spin_unlock_bh);

#ifndef CONFIG_PREEMPT_RT

noinline int __lockfunc _raw_read_trylock(rwlock_t *lock)
{
	return __raw_read_trylock(lock);
}
EXPORT_SYMBOL(_raw_read_trylock);

noinline void __lockfunc _raw_read_lock(rwlock_t *lock)
{
	__raw_read_lock(lock);
}
EXPORT_SYMBOL(_raw_read_lock);

noinline unsigned long __lockfunc _raw_read_lock_irqsave(rwlock_t *lock)
{
	return __raw_read_lock_irqsave(lock);
}
EXPORT_SYMBOL(_raw_read_lock_irqsave);

noinline void __lockfunc _raw_read_lock_irq(rwlock_t *lock)
{
	__raw_read_lock_irq(lock);
}
EXPORT_SYMBOL(_raw_read_lock_irq);

noinline void __lockfunc _raw_read_lock_bh(rwlock_t *lock)
{
	__raw_read_lock_bh(lock);
}
EXPORT_SYMBOL(_raw_read_lock_bh);

noinline void __lockfunc _raw_read_unlock(rwlock_t *lock)
{
	__raw_read_unlock(lock);
}
EXPORT_SYMBOL(_raw_read_unlock);

noinline void __lockfunc _raw_read_unlock_irqrestore(rwlock_t *lock, unsigned long flags)
{
	__raw_read_unlock_irqrestore(lock, flags);
}
EXPORT_SYMBOL(_raw_read_unlock_irqrestore);

noinline void __lockfunc _raw_read_unlock_irq(rwlock_t *lock)
{
	__raw_read_unlock_irq(lock);
}
EXPORT_SYMBOL(_raw_read_unlock_irq);

noinline void __lockfunc _raw_read_unlock_bh(rwlock_t *lock)
{
	__raw_read_unlock_bh(lock);
}
EXPORT_SYMBOL(_raw_read_unlock_bh);

noinline int __lockfunc _raw_write_trylock(rwlock_t *lock)
{
	return __raw_write_trylock(lock);
}
EXPORT_SYMBOL(_raw_write_trylock);

noinline void __lockfunc _raw_write_lock(rwlock_t *lock)
{
	__raw_write_lock(lock);
}
EXPORT_SYMBOL(_raw_write_lock);

#define __raw_write_lock_nested(lock, subclass)	__raw_write_lock(((void)(subclass), (lock)))

void __lockfunc _raw_write_lock_nested(rwlock_t *lock, int subclass)
{
	__raw_write_lock_nested(lock, subclass);
}
EXPORT_SYMBOL(_raw_write_lock_nested);

noinline unsigned long __lockfunc _raw_write_lock_irqsave(rwlock_t *lock)
{
	return __raw_write_lock_irqsave(lock);
}
EXPORT_SYMBOL(_raw_write_lock_irqsave);

noinline void __lockfunc _raw_write_lock_irq(rwlock_t *lock)
{
	__raw_write_lock_irq(lock);
}
EXPORT_SYMBOL(_raw_write_lock_irq);

noinline void __lockfunc _raw_write_lock_bh(rwlock_t *lock)
{
	__raw_write_lock_bh(lock);
}
EXPORT_SYMBOL(_raw_write_lock_bh);

noinline void __lockfunc _raw_write_unlock(rwlock_t *lock)
{
	__raw_write_unlock(lock);
}
EXPORT_SYMBOL(_raw_write_unlock);

noinline void __lockfunc _raw_write_unlock_irqrestore(rwlock_t *lock, unsigned long flags)
{
	__raw_write_unlock_irqrestore(lock, flags);
}
EXPORT_SYMBOL(_raw_write_unlock_irqrestore);

noinline void __lockfunc _raw_write_unlock_irq(rwlock_t *lock)
{
	__raw_write_unlock_irq(lock);
}
EXPORT_SYMBOL(_raw_write_unlock_irq);

noinline void __lockfunc _raw_write_unlock_bh(rwlock_t *lock)
{
	__raw_write_unlock_bh(lock);
}
EXPORT_SYMBOL(_raw_write_unlock_bh);

#endif /* !CONFIG_PREEMPT_RT */


notrace int in_lock_functions(unsigned long addr)
{
	/* Linker adds these: start and end of __lockfunc functions */
	extern char __lock_text_start[], __lock_text_end[];

	return addr >= (unsigned long)__lock_text_start
	&& addr < (unsigned long)__lock_text_end;
}
EXPORT_SYMBOL(in_lock_functions);

#if defined(CONFIG_PROVE_LOCKING) && defined(CONFIG_PREEMPT_RT)
void notrace lockdep_assert_in_softirq_func(void)
{
	lockdep_assert_in_softirq();
}
EXPORT_SYMBOL(lockdep_assert_in_softirq_func);
#endif
