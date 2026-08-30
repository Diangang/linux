/* SPDX-License-Identifier: GPL-2.0 */
/*
 * kernel/lockdep_internals.h
 *
 * Runtime locking correctness validator
 *
 * lockdep subsystem internal functions and variables.
 */

/*
 * Lock-class usage-state bits:
 */
enum lock_usage_bit {
#define LOCKDEP_STATE(__STATE)		\
	LOCK_USED_IN_##__STATE,		\
	LOCK_USED_IN_##__STATE##_READ,	\
	LOCK_ENABLED_##__STATE,		\
	LOCK_ENABLED_##__STATE##_READ,
#include "lockdep_states.h"
#undef LOCKDEP_STATE
	LOCK_USED,
	LOCK_USED_READ,
	LOCK_USAGE_STATES,
};

/* states after LOCK_USED_READ are not traced and printed */
static_assert(LOCK_TRACE_STATES == LOCK_USAGE_STATES);

#define LOCK_USAGE_READ_MASK 1
#define LOCK_USAGE_DIR_MASK  2
#define LOCK_USAGE_STATE_MASK (~(LOCK_USAGE_READ_MASK | LOCK_USAGE_DIR_MASK))

/*
 * Usage-state bitmasks:
 */
#define __LOCKF(__STATE)	LOCKF_##__STATE = (1 << LOCK_##__STATE),

enum {
#define LOCKDEP_STATE(__STATE)						\
	__LOCKF(USED_IN_##__STATE)					\
	__LOCKF(USED_IN_##__STATE##_READ)				\
	__LOCKF(ENABLED_##__STATE)					\
	__LOCKF(ENABLED_##__STATE##_READ)
#include "lockdep_states.h"
#undef LOCKDEP_STATE
	__LOCKF(USED)
	__LOCKF(USED_READ)
};

enum {
#define LOCKDEP_STATE(__STATE)	LOCKF_ENABLED_##__STATE |
	LOCKF_ENABLED_IRQ =
#include "lockdep_states.h"
	0,
#undef LOCKDEP_STATE

#define LOCKDEP_STATE(__STATE)	LOCKF_USED_IN_##__STATE |
	LOCKF_USED_IN_IRQ =
#include "lockdep_states.h"
	0,
#undef LOCKDEP_STATE

#define LOCKDEP_STATE(__STATE)	LOCKF_ENABLED_##__STATE##_READ |
	LOCKF_ENABLED_IRQ_READ =
#include "lockdep_states.h"
	0,
#undef LOCKDEP_STATE

#define LOCKDEP_STATE(__STATE)	LOCKF_USED_IN_##__STATE##_READ |
	LOCKF_USED_IN_IRQ_READ =
#include "lockdep_states.h"
	0,
#undef LOCKDEP_STATE
};

#define LOCKF_ENABLED_IRQ_ALL (LOCKF_ENABLED_IRQ | LOCKF_ENABLED_IRQ_READ)
#define LOCKF_USED_IN_IRQ_ALL (LOCKF_USED_IN_IRQ | LOCKF_USED_IN_IRQ_READ)

#define LOCKF_IRQ (LOCKF_ENABLED_IRQ | LOCKF_USED_IN_IRQ)
#define LOCKF_IRQ_READ (LOCKF_ENABLED_IRQ_READ | LOCKF_USED_IN_IRQ_READ)

/*
 * CONFIG_LOCKDEP_SMALL is defined for sparc. Sparc requires .text,
 * .data and .bss to fit in required 32MB limit for the kernel. With
 * CONFIG_LOCKDEP we could go over this limit and cause system boot-up problems.
 * So, reduce the static allocations for lockdeps related structures so that
 * everything fits in current required size limit.
 */
#define MAX_LOCKDEP_ENTRIES	(1UL << CONFIG_LOCKDEP_BITS)

#define MAX_LOCKDEP_CHAINS_BITS	CONFIG_LOCKDEP_CHAINS_BITS

/*
 * Stack-trace: tightly packed array of stack backtrace
 * addresses. Protected by the hash_lock.
 */
#define MAX_STACK_TRACE_ENTRIES	(1UL << CONFIG_LOCKDEP_STACK_TRACE_BITS)
#define STACK_TRACE_HASH_SIZE	(1 << CONFIG_LOCKDEP_STACK_TRACE_HASH_BITS)

/*
 * Bit definitions for lock_chain.irq_context
 */
#define LOCK_CHAIN_SOFTIRQ_CONTEXT	(1 << 0)
#define LOCK_CHAIN_HARDIRQ_CONTEXT	(1 << 1)

#define MAX_LOCKDEP_CHAINS	(1UL << MAX_LOCKDEP_CHAINS_BITS)

#define AVG_LOCKDEP_CHAIN_DEPTH		5
#define MAX_LOCKDEP_CHAIN_HLOCKS (MAX_LOCKDEP_CHAINS * AVG_LOCKDEP_CHAIN_DEPTH)

extern struct lock_chain lock_chains[];

#define LOCK_USAGE_CHARS (2*XXX_LOCK_USAGE_STATES + 1)

extern void get_usage_chars(struct lock_class *class,
			    char usage[LOCK_USAGE_CHARS]);

extern const char *__get_key_name(const struct lockdep_subclass_key *key,
				  char *str);

struct lock_class *lock_chain_get_class(struct lock_chain *chain, int i);

extern unsigned long nr_lock_classes;
extern unsigned long nr_zapped_classes;
extern unsigned long nr_zapped_lock_chains;
extern unsigned long nr_list_entries;
extern unsigned long nr_dynamic_keys;
long lockdep_next_lockchain(long i);
unsigned long lock_chain_count(void);
extern unsigned long nr_stack_trace_entries;

extern unsigned int nr_hardirq_chains;
extern unsigned int nr_softirq_chains;
extern unsigned int nr_process_chains;
extern unsigned int nr_free_chain_hlocks;
extern unsigned int nr_lost_chain_hlocks;
extern unsigned int nr_large_chain_blocks;

extern unsigned int max_lockdep_depth;
extern unsigned int max_bfs_queue_depth;
extern unsigned long max_lock_class_idx;

extern struct lock_class lock_classes[MAX_LOCKDEP_KEYS];
extern unsigned long lock_classes_in_use[];

static inline unsigned long
lockdep_count_forward_deps(struct lock_class *class)
{
	return 0;
}
static inline unsigned long
lockdep_count_backward_deps(struct lock_class *class)
{
	return 0;
}

# define __debug_atomic_inc(ptr)	do { } while (0)
# define debug_atomic_inc(ptr)		do { } while (0)
# define debug_atomic_dec(ptr)		do { } while (0)
# define debug_atomic_read(ptr)		0
# define debug_class_ops_inc(ptr)	do { } while (0)
