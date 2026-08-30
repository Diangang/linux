/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Runtime locking correctness validator
 *
 *  Copyright (C) 2006,2007 Red Hat, Inc., Ingo Molnar <mingo@redhat.com>
 *  Copyright (C) 2007 Red Hat, Inc., Peter Zijlstra
 *
 * see Documentation/locking/lockdep-design.rst for more details.
 */
#ifndef __LINUX_LOCKDEP_TYPES_H
#define __LINUX_LOCKDEP_TYPES_H

#include <linux/types.h>

#define MAX_LOCKDEP_SUBCLASSES		8UL

enum lockdep_wait_type {
	LD_WAIT_INV = 0,	/* not checked, catch all */

	LD_WAIT_FREE,		/* wait free, rcu etc.. */
	LD_WAIT_SPIN,		/* spin loops, raw_spinlock_t etc.. */

	LD_WAIT_CONFIG = LD_WAIT_SPIN,
	LD_WAIT_SLEEP,		/* sleeping locks, mutex_t etc.. */

	LD_WAIT_MAX,		/* must be last */
};

enum lockdep_lock_type {
	LD_LOCK_NORMAL = 0,	/* normal, catch all */
	LD_LOCK_PERCPU,		/* percpu */
	LD_LOCK_WAIT_OVERRIDE,	/* annotation */
	LD_LOCK_MAX,
};


/*
 * The class key takes no space if lockdep is disabled:
 */
struct lock_class_key { };

/*
 * The lockdep_map takes no space if lockdep is disabled:
 */
struct lockdep_map { };

struct pin_cookie { };


#endif /* __LINUX_LOCKDEP_TYPES_H */
