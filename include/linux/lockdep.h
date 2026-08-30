/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Runtime locking correctness validator
 *
 *  Copyright (C) 2006,2007 Red Hat, Inc., Ingo Molnar <mingo@redhat.com>
 *  Copyright (C) 2007 Red Hat, Inc., Peter Zijlstra
 *
 * see Documentation/locking/lockdep-design.rst for more details.
 */
#ifndef __LINUX_LOCKDEP_H
#define __LINUX_LOCKDEP_H

#include <linux/lockdep_types.h>
#include <linux/smp.h>
#include <asm/percpu.h>

struct task_struct;


static inline void lockdep_init_task(struct task_struct *task)
{
}

static inline void lockdep_off(void)
{
}

static inline void lockdep_on(void)
{
}

static inline void lockdep_set_selftest_task(struct task_struct *task)
{
}

# define lock_acquire(l, s, t, r, c, n, i)	do { } while (0)
# define lock_release(l, i)			do { } while (0)
# define lock_downgrade(l, i)			do { } while (0)
# define lock_set_class(l, n, key, s, i)	do { (void)(key); } while (0)
# define lock_set_novalidate_class(l, n, i)	do { } while (0)
# define lock_set_subclass(l, s, i)		do { } while (0)
# define lockdep_init()				do { } while (0)
# define lockdep_init_map_type(lock, name, key, sub, inner, outer, type) \
		do { (void)(name); (void)(key); } while (0)
# define lockdep_init_map_waits(lock, name, key, sub, inner, outer) \
		do { (void)(name); (void)(key); } while (0)
# define lockdep_init_map_wait(lock, name, key, sub, inner) \
		do { (void)(name); (void)(key); } while (0)
# define lockdep_init_map(lock, name, key, sub) \
		do { (void)(name); (void)(key); } while (0)
# define lockdep_set_class(lock, key)		do { (void)(key); } while (0)
# define lockdep_set_class_and_name(lock, key, name) \
		do { (void)(key); (void)(name); } while (0)
#define lockdep_set_class_and_subclass(lock, key, sub) \
		do { (void)(key); } while (0)
#define lockdep_set_subclass(lock, sub)		do { } while (0)

#define lockdep_set_novalidate_class(lock) do { } while (0)
#define lockdep_set_notrack_class(lock) do { } while (0)

/*
 * We don't define lockdep_match_class() and lockdep_match_key() for !LOCKDEP
 * case since the result is not well defined and the caller should rather
 * #ifdef the call himself.
 */

# define lockdep_reset()		do { debug_locks = 1; } while (0)
# define lockdep_free_key_range(start, size)	do { } while (0)
# define lockdep_sys_exit() 			do { } while (0)

static inline void lockdep_register_key(struct lock_class_key *key)
{
}

static inline void lockdep_unregister_key(struct lock_class_key *key)
{
}

#define lockdep_depth(tsk)	(0)

/*
 * Dummy forward declarations, allow users to write less ifdef-y code
 * and depend on dead code elimination.
 */
extern int lock_is_held(const void *);
extern int lockdep_is_held(const void *);
#define lockdep_is_held_type(l, r)		(1)

#define lockdep_assert(c)			do { } while (0)
#define lockdep_assert_once(c)			do { } while (0)

#define lockdep_assert_held(l)			__assume_ctx_lock(l)
#define lockdep_assert_not_held(l)		do { (void)(l); } while (0)
#define lockdep_assert_held_write(l)		__assume_ctx_lock(l)
#define lockdep_assert_held_read(l)		__assume_shared_ctx_lock(l)
#define lockdep_assert_held_once(l)		do { (void)(l); } while (0)
#define lockdep_assert_none_held_once()	do { } while (0)

#define lockdep_recursing(tsk)			(0)

#define NIL_COOKIE (struct pin_cookie){ }

#define lockdep_pin_lock(l)			({ struct pin_cookie cookie = { }; cookie; })
#define lockdep_repin_lock(l, c)		do { (void)(l); (void)(c); } while (0)
#define lockdep_unpin_lock(l, c)		do { (void)(l); (void)(c); } while (0)

#define DEFINE_WAIT_OVERRIDE_MAP(_name, _wait_type)	\
	struct lockdep_map __maybe_unused _name = {}


#define lock_set_cmp_fn(lock, ...)	do { } while (0)

enum xhlock_context_t {
	XHLOCK_HARD,
	XHLOCK_SOFT,
	XHLOCK_CTX_NR,
};

/*
 * To initialize a lockdep_map statically use this macro.
 * Note that _name must not be NULL.
 */
#define STATIC_LOCKDEP_MAP_INIT(_name, _key) \
	{ .name = (_name), .key = (void *)(_key), }

static inline void lockdep_invariant_state(bool force) {}
static inline void lockdep_free_task(struct task_struct *task) {}


#define lock_contended(lockdep_map, ip) do {} while (0)
#define lock_acquired(lockdep_map, ip) do {} while (0)

#define LOCK_CONTENDED(_lock, try, lock) \
	lock(_lock)

#define LOCK_CONTENDED_RETURN(_lock, try, lock) \
	lock(_lock)


static inline void print_irqtrace_events(struct task_struct *curr)
{
}

/* Variable used to make lockdep treat read_lock() as recursive in selftests */
#define force_read_lock_recursive 0

/* If !LOCKDEP, the value is meaningless */
#define read_lock_is_recursive() 0

/*
 * For trivial one-depth nesting of a lock-class, the following
 * global define can be used. (Subsystems with multiple levels
 * of nesting should define their own lock-nesting subclasses.)
 */
#define SINGLE_DEPTH_NESTING			1

/*
 * Map the dependency ops to NOP or to real lockdep ops, depending
 * on the per lock-class debug mode:
 */

#define lock_acquire_exclusive(l, s, t, n, i)		lock_acquire(l, s, t, 0, 1, n, i)
#define lock_acquire_shared(l, s, t, n, i)		lock_acquire(l, s, t, 1, 1, n, i)
#define lock_acquire_shared_recursive(l, s, t, n, i)	lock_acquire(l, s, t, 2, 1, n, i)

#define spin_acquire(l, s, t, i)		lock_acquire_exclusive(l, s, t, NULL, i)
#define spin_acquire_nest(l, s, t, n, i)	lock_acquire_exclusive(l, s, t, n, i)
#define spin_release(l, i)			lock_release(l, i)

#define rwlock_acquire(l, s, t, i)		lock_acquire_exclusive(l, s, t, NULL, i)
#define rwlock_acquire_read(l, s, t, i)					\
do {									\
	if (read_lock_is_recursive())					\
		lock_acquire_shared_recursive(l, s, t, NULL, i);	\
	else								\
		lock_acquire_shared(l, s, t, NULL, i);			\
} while (0)

#define rwlock_release(l, i)			lock_release(l, i)

#define seqcount_acquire(l, s, t, i)		lock_acquire_exclusive(l, s, t, NULL, i)
#define seqcount_acquire_read(l, s, t, i)	lock_acquire_shared_recursive(l, s, t, NULL, i)
#define seqcount_release(l, i)			lock_release(l, i)

#define mutex_acquire(l, s, t, i)		lock_acquire_exclusive(l, s, t, NULL, i)
#define mutex_acquire_nest(l, s, t, n, i)	lock_acquire_exclusive(l, s, t, n, i)
#define mutex_release(l, i)			lock_release(l, i)

#define rwsem_acquire(l, s, t, i)		lock_acquire_exclusive(l, s, t, NULL, i)
#define rwsem_acquire_nest(l, s, t, n, i)	lock_acquire_exclusive(l, s, t, n, i)
#define rwsem_acquire_read(l, s, t, i)		lock_acquire_shared(l, s, t, NULL, i)
#define rwsem_release(l, i)			lock_release(l, i)

#define lock_map_acquire(l)			lock_acquire_exclusive(l, 0, 0, NULL, _THIS_IP_)
#define lock_map_acquire_try(l)			lock_acquire_exclusive(l, 0, 1, NULL, _THIS_IP_)
#define lock_map_acquire_read(l)		lock_acquire_shared_recursive(l, 0, 0, NULL, _THIS_IP_)
#define lock_map_acquire_tryread(l)		lock_acquire_shared_recursive(l, 0, 1, NULL, _THIS_IP_)
#define lock_map_release(l)			lock_release(l, _THIS_IP_)
#define lock_map_sync(l)			lock_sync(l, 0, 0, 1, NULL, _THIS_IP_)

# define might_lock(lock) do { } while (0)
# define might_lock_read(lock) do { } while (0)
# define might_lock_nested(lock, subclass) do { } while (0)

# define lockdep_assert_irqs_enabled() do { } while (0)
# define lockdep_assert_irqs_disabled() do { } while (0)
# define lockdep_assert_in_irq() do { } while (0)
# define lockdep_assert_no_hardirq() do { } while (0)

# define lockdep_assert_preemption_enabled() do { } while (0)
# define lockdep_assert_preemption_disabled() do { } while (0)
# define lockdep_assert_in_softirq() do { } while (0)
# define lockdep_assert_in_softirq_func() do { } while (0)


# define lockdep_assert_RT_in_threaded_ctx() do { } while (0)


static inline void
lockdep_rcu_suspicious(const char *file, const int line, const char *s)
{
}

#endif /* __LINUX_LOCKDEP_H */
