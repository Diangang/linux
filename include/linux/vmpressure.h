/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __LINUX_VMPRESSURE_H
#define __LINUX_VMPRESSURE_H

#include <linux/mutex.h>
#include <linux/list.h>
#include <linux/workqueue.h>
#include <linux/gfp.h>
#include <linux/types.h>
#include <linux/cgroup.h>
#include <linux/eventfd.h>

struct vmpressure {
	unsigned long scanned;
	unsigned long reclaimed;

	unsigned long tree_scanned;
	unsigned long tree_reclaimed;
	/* The lock is used to keep the scanned/reclaimed above in sync. */
	spinlock_t sr_lock;

	/* The list of vmpressure_event structs. */
	struct list_head events;
	/* Have to grab the lock on events traversal or modifications. */
	struct mutex events_lock;

	struct work_struct work;
};

struct mem_cgroup;

static inline void vmpressure(gfp_t gfp, struct mem_cgroup *memcg, bool tree,
			      unsigned long scanned, unsigned long reclaimed) {}
static inline void vmpressure_prio(gfp_t gfp, struct mem_cgroup *memcg,
				   int prio) {}
#endif /* __LINUX_VMPRESSURE_H */
