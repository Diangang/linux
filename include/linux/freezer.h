/* SPDX-License-Identifier: GPL-2.0 */
/* Compatibility helpers for retained callers. */

#ifndef FREEZER_H_INCLUDED
#define FREEZER_H_INCLUDED

#include <linux/types.h>

struct task_struct;

static inline bool freezing(struct task_struct *p) { return false; }
static inline bool try_to_freeze(void) { return false; }

#endif	/* FREEZER_H_INCLUDED */
