/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_LOCAL_LOCK_H
# error "Do not include directly, include linux/local_lock.h"
#endif

#include <linux/percpu-defs.h>
#include <linux/irqflags.h>
#include <linux/lockdep.h>
#include <linux/debug_locks.h>
#include <asm/current.h>


context_lock_struct(local_lock) {
};
typedef struct local_lock local_lock_t;

/* local_trylock() and local_trylock_irqsave() only work with local_trylock_t */
context_lock_struct(local_trylock) {
	u8		acquired;
};
typedef struct local_trylock local_trylock_t;

# define LOCAL_LOCK_DEBUG_INIT(lockname)
# define LOCAL_TRYLOCK_DEBUG_INIT(lockname)
static inline void local_lock_acquire(local_lock_t *l) { }
static inline void local_trylock_acquire(local_lock_t *l) { }
static inline void local_lock_release(local_lock_t *l) { }
static inline void local_lock_debug_init(local_lock_t *l) { }

#define INIT_LOCAL_LOCK(lockname)	{ LOCAL_LOCK_DEBUG_INIT(lockname) }
#define INIT_LOCAL_TRYLOCK(lockname)	{ LOCAL_TRYLOCK_DEBUG_INIT(lockname) }

#define __local_lock_init(lock)					\
do {								\
	static struct lock_class_key __key;			\
								\
	debug_check_no_locks_freed((void *)lock, sizeof(*lock));\
	lockdep_init_map_type(&(lock)->dep_map, #lock, &__key,  \
			      0, LD_WAIT_CONFIG, LD_WAIT_INV,	\
			      LD_LOCK_PERCPU);			\
	local_lock_debug_init(lock);				\
} while (0)

#define __local_trylock_init(lock)				\
do {								\
	__local_lock_init((local_lock_t *)lock);		\
} while (0)

#define __spinlock_nested_bh_init(lock)				\
do {								\
	static struct lock_class_key __key;			\
								\
	debug_check_no_locks_freed((void *)lock, sizeof(*lock));\
	lockdep_init_map_type(&(lock)->dep_map, #lock, &__key,  \
			      0, LD_WAIT_CONFIG, LD_WAIT_INV,	\
			      LD_LOCK_NORMAL);			\
	local_lock_debug_init(lock);				\
} while (0)

#define __local_lock_acquire(lock)					\
	do {								\
		local_trylock_t *__tl;					\
		local_lock_t *__l;					\
									\
		__l = (local_lock_t *)(lock);				\
		__tl = (local_trylock_t *)__l;				\
		_Generic((lock),					\
			local_trylock_t *: ({				\
				lockdep_assert(__tl->acquired == 0);	\
				WRITE_ONCE(__tl->acquired, 1);		\
			}),						\
			local_lock_t *: (void)0);			\
		local_lock_acquire(__l);				\
	} while (0)

#define __local_lock(lock)					\
	do {							\
		preempt_disable();				\
		__local_lock_acquire(lock);			\
		__acquire(lock);				\
	} while (0)

#define __local_lock_irq(lock)					\
	do {							\
		local_irq_disable();				\
		__local_lock_acquire(lock);			\
		__acquire(lock);				\
	} while (0)

#define __local_lock_irqsave(lock, flags)			\
	do {							\
		local_irq_save(flags);				\
		__local_lock_acquire(lock);			\
		__acquire(lock);				\
	} while (0)

#define __local_trylock(lock)					\
	__try_acquire_ctx_lock(lock, ({				\
		local_trylock_t *__tl;				\
								\
		preempt_disable();				\
		__tl = (lock);					\
		if (READ_ONCE(__tl->acquired)) {		\
			preempt_enable();			\
			__tl = NULL;				\
		} else {					\
			WRITE_ONCE(__tl->acquired, 1);		\
			local_trylock_acquire(			\
				(local_lock_t *)__tl);		\
		}						\
		!!__tl;						\
	}))

#define __local_trylock_irqsave(lock, flags)			\
	__try_acquire_ctx_lock(lock, ({				\
		local_trylock_t *__tl;				\
								\
		local_irq_save(flags);				\
		__tl = (lock);					\
		if (READ_ONCE(__tl->acquired)) {		\
			local_irq_restore(flags);		\
			__tl = NULL;				\
		} else {					\
			WRITE_ONCE(__tl->acquired, 1);		\
			local_trylock_acquire(			\
				(local_lock_t *)__tl);		\
		}						\
		!!__tl;						\
	}))

/* preemption or migration must be disabled before calling __local_lock_is_locked */
#define __local_lock_is_locked(lock) READ_ONCE(this_cpu_ptr(lock)->acquired)

#define __local_lock_release(lock)					\
	do {								\
		local_trylock_t *__tl;					\
		local_lock_t *__l;					\
									\
		__l = (local_lock_t *)(lock);				\
		__tl = (local_trylock_t *)__l;				\
		local_lock_release(__l);				\
		_Generic((lock),					\
			local_trylock_t *: ({				\
				lockdep_assert(__tl->acquired == 1);	\
				WRITE_ONCE(__tl->acquired, 0);		\
			}),						\
			local_lock_t *: (void)0);			\
	} while (0)

#define __local_unlock(lock)					\
	do {							\
		__release(lock);				\
		__local_lock_release(lock);			\
		preempt_enable();				\
	} while (0)

#define __local_unlock_irq(lock)				\
	do {							\
		__release(lock);				\
		__local_lock_release(lock);			\
		local_irq_enable();				\
	} while (0)

#define __local_unlock_irqrestore(lock, flags)			\
	do {							\
		__release(lock);				\
		__local_lock_release(lock);			\
		local_irq_restore(flags);			\
	} while (0)

#define __local_lock_nested_bh(lock)				\
	do {							\
		lockdep_assert_in_softirq();			\
		local_lock_acquire((lock));			\
		__acquire(lock);				\
	} while (0)

#define __local_unlock_nested_bh(lock)				\
	do {							\
		__release(lock);				\
		local_lock_release((lock));			\
	} while (0)


#if defined(WARN_CONTEXT_ANALYSIS) && !defined(__CHECKER__)
/*
 * Because the compiler only knows about the base per-CPU variable, use this
 * helper function to make the compiler think we lock/unlock the @base variable,
 * and hide the fact we actually pass the per-CPU instance to lock/unlock
 * functions.
 */
static __always_inline local_lock_t *__this_cpu_local_lock(local_lock_t __percpu *base)
	__returns_ctx_lock(base) __attribute__((overloadable))
{
	return this_cpu_ptr(base);
}
static __always_inline local_trylock_t *__this_cpu_local_lock(local_trylock_t __percpu *base)
	__returns_ctx_lock(base) __attribute__((overloadable))
{
	return this_cpu_ptr(base);
}
#else  /* WARN_CONTEXT_ANALYSIS */
#define __this_cpu_local_lock(base) this_cpu_ptr(base)
#endif /* WARN_CONTEXT_ANALYSIS */
