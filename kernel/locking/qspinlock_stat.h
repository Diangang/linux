/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 *
 * Authors: Waiman Long <longman@redhat.com>
 */

#include "lock_events.h"

#if 0

#else /* CONFIG_LOCK_EVENT_COUNTS */

static inline void lockevent_pv_hop(int hopcnt)	{ }

#endif /* CONFIG_LOCK_EVENT_COUNTS */
