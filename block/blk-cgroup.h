/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _BLK_CGROUP_PRIVATE_H
#define _BLK_CGROUP_PRIVATE_H
/*
 * block cgroup private header
 *
 * Based on ideas and code from CFQ, CFS and BFQ:
 * Copyright (C) 2003 Jens Axboe <axboe@kernel.dk>
 *
 * Copyright (C) 2008 Fabio Checconi <fabio@gandalf.sssup.it>
 *		      Paolo Valente <paolo.valente@unimore.it>
 *
 * Copyright (C) 2009 Vivek Goyal <vgoyal@redhat.com>
 * 	              Nauman Rafique <nauman@google.com>
 */

#include <linux/blk-cgroup.h>
#include <linux/cgroup.h>
#include <linux/kthread.h>
#include <linux/blk-mq.h>
#include <linux/llist.h>
#include "blk.h"

struct blkcg_gq;
struct blkg_policy_data;


/* percpu_counter batch for blkg_[rw]stats, per-cpu drift doesn't matter */
#define BLKG_STAT_CPU_BATCH	(INT_MAX / 2)


struct blkg_policy_data {
};

struct blkcg_policy_data {
};

struct blkcg_policy {
};

struct blkcg {
};

static inline struct blkcg_gq *blkg_lookup(struct blkcg *blkcg, void *key) { return NULL; }
static inline void blkg_init_queue(struct request_queue *q) { }
static inline int blkcg_init_disk(struct gendisk *disk) { return 0; }
static inline void blkcg_exit_disk(struct gendisk *disk) { }
static inline int blkcg_policy_register(struct blkcg_policy *pol) { return 0; }
static inline void blkcg_policy_unregister(struct blkcg_policy *pol) { }
static inline int blkcg_activate_policy(struct gendisk *disk,
					const struct blkcg_policy *pol) { return 0; }
static inline void blkcg_deactivate_policy(struct gendisk *disk,
					   const struct blkcg_policy *pol) { }

static inline struct blkg_policy_data *blkg_to_pd(struct blkcg_gq *blkg,
						  struct blkcg_policy *pol) { return NULL; }
static inline struct blkcg_gq *pd_to_blkg(struct blkg_policy_data *pd) { return NULL; }
static inline void blkg_get(struct blkcg_gq *blkg) { }
static inline void blkg_put(struct blkcg_gq *blkg) { }
static inline void blk_cgroup_bio_start(struct bio *bio) { }
static inline bool blk_cgroup_mergeable(struct request *rq, struct bio *bio) { return true; }

#define blk_queue_for_each_rl(rl, q)	\
	for ((rl) = &(q)->root_rl; (rl); (rl) = NULL)


#endif /* _BLK_CGROUP_PRIVATE_H */
