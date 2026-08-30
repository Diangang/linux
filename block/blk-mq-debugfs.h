/* SPDX-License-Identifier: GPL-2.0 */
#ifndef INT_BLK_MQ_DEBUGFS_H
#define INT_BLK_MQ_DEBUGFS_H

static inline void blk_mq_debugfs_register(struct request_queue *q)
{
}

static inline void blk_mq_debugfs_register_hctx(struct request_queue *q,
						struct blk_mq_hw_ctx *hctx)
{
}

static inline void blk_mq_debugfs_unregister_hctx(struct blk_mq_hw_ctx *hctx)
{
}

static inline void blk_mq_debugfs_register_hctxs(struct request_queue *q)
{
}

static inline void blk_mq_debugfs_unregister_hctxs(struct request_queue *q)
{
}

static inline void blk_mq_debugfs_register_sched(struct request_queue *q)
{
}

static inline void blk_mq_debugfs_unregister_sched(struct request_queue *q)
{
}

static inline void blk_mq_debugfs_register_sched_hctx(struct request_queue *q,
						      struct blk_mq_hw_ctx *hctx)
{
}

static inline void blk_mq_debugfs_unregister_sched_hctx(struct blk_mq_hw_ctx *hctx)
{
}

static inline void blk_mq_debugfs_register_rq_qos(struct request_queue *q)
{
}


static inline int queue_zone_wplugs_show(void *data, struct seq_file *m)
{
	return 0;
}

#endif
