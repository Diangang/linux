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


/*
 * If lockdep is enabled then we use the non-preemption spin-ops
 * even on CONFIG_PREEMPT, because lockdep assumes that interrupts are
 * not re-enabled during lock-acquire (which the preempt-spin-ops do):
 */
/*
 * The __lock_function inlines are taken from
 * spinlock : include/linux/spinlock_api_smp.h
 * rwlock   : include/linux/rwlock_api_smp.h
 */

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



notrace int in_lock_functions(unsigned long addr)
{
	/* Linker adds these: start and end of __lockfunc functions */
	extern char __lock_text_start[], __lock_text_end[];

	return addr >= (unsigned long)__lock_text_start
	&& addr < (unsigned long)__lock_text_end;
}
EXPORT_SYMBOL(in_lock_functions);
