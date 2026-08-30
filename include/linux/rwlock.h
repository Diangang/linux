#ifndef __LINUX_RWLOCK_H
#define __LINUX_RWLOCK_H

#ifndef __LINUX_INSIDE_SPINLOCK_H
# error "Please do not include this file directly."
#endif

/*
 * rwlock related methods
 *
 * split out from spinlock.h
 *
 * portions Copyright 2005, Red Hat, Inc., Ingo Molnar
 * Released under the General Public License (GPL).
 */

# define rwlock_init(lock)					\
	do { *(lock) = __RW_LOCK_UNLOCKED(lock); } while (0)

# define do_raw_read_lock(rwlock)	do {__acquire_shared(lock); arch_read_lock(&(rwlock)->raw_lock); } while (0)
static inline int do_raw_read_trylock(rwlock_t *rwlock)
	__cond_acquires_shared(true, rwlock)
	__no_context_analysis
{
	return arch_read_trylock(&(rwlock)->raw_lock);
}
# define do_raw_read_unlock(rwlock)	do {arch_read_unlock(&(rwlock)->raw_lock); __release_shared(lock); } while (0)
# define do_raw_write_lock(rwlock)	do {__acquire(lock); arch_write_lock(&(rwlock)->raw_lock); } while (0)
static inline int do_raw_write_trylock(rwlock_t *rwlock)
	__cond_acquires(true, rwlock)
	__no_context_analysis
{
	return arch_write_trylock(&(rwlock)->raw_lock);
}
# define do_raw_write_unlock(rwlock)	do {arch_write_unlock(&(rwlock)->raw_lock); __release(lock); } while (0)

/*
 * Define the various rw_lock methods.  Note we define these
 * regardless of whether CONFIG_SMP or CONFIG_PREEMPT are set. The various
 * methods are defined as nops in the case they are not required.
 */
#define read_trylock(lock)	_raw_read_trylock(lock)
#define write_trylock(lock)	_raw_write_trylock(lock)

#define write_lock(lock)	_raw_write_lock(lock)
#define read_lock(lock)		_raw_read_lock(lock)

#define write_lock_nested(lock, subclass)	_raw_write_lock(lock)

#if defined(CONFIG_SMP)

#define read_lock_irqsave(lock, flags)			\
	do {						\
		typecheck(unsigned long, flags);	\
		flags = _raw_read_lock_irqsave(lock);	\
	} while (0)
#define write_lock_irqsave(lock, flags)			\
	do {						\
		typecheck(unsigned long, flags);	\
		flags = _raw_write_lock_irqsave(lock);	\
	} while (0)

#else

#define read_lock_irqsave(lock, flags)			\
	do {						\
		typecheck(unsigned long, flags);	\
		_raw_read_lock_irqsave(lock, flags);	\
	} while (0)
#define write_lock_irqsave(lock, flags)			\
	do {						\
		typecheck(unsigned long, flags);	\
		_raw_write_lock_irqsave(lock, flags);	\
	} while (0)

#endif

#define read_lock_irq(lock)		_raw_read_lock_irq(lock)
#define read_lock_bh(lock)		_raw_read_lock_bh(lock)
#define write_lock_irq(lock)		_raw_write_lock_irq(lock)
#define write_lock_bh(lock)		_raw_write_lock_bh(lock)
#define read_unlock(lock)		_raw_read_unlock(lock)
#define write_unlock(lock)		_raw_write_unlock(lock)
#define read_unlock_irq(lock)		_raw_read_unlock_irq(lock)
#define write_unlock_irq(lock)		_raw_write_unlock_irq(lock)

#define read_unlock_irqrestore(lock, flags)			\
	do {							\
		typecheck(unsigned long, flags);		\
		_raw_read_unlock_irqrestore(lock, flags);	\
	} while (0)
#define read_unlock_bh(lock)		_raw_read_unlock_bh(lock)

#define write_unlock_irqrestore(lock, flags)		\
	do {						\
		typecheck(unsigned long, flags);	\
		_raw_write_unlock_irqrestore(lock, flags);	\
	} while (0)
#define write_unlock_bh(lock)		_raw_write_unlock_bh(lock)

#define write_trylock_irqsave(lock, flags) _raw_write_trylock_irqsave(lock, &(flags))

#ifdef arch_rwlock_is_contended
#define rwlock_is_contended(lock) \
	 arch_rwlock_is_contended(&(lock)->raw_lock)
#else
#define rwlock_is_contended(lock)	((void)(lock), 0)
#endif /* arch_rwlock_is_contended */

#endif /* __LINUX_RWLOCK_H */
