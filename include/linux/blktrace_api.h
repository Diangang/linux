/* SPDX-License-Identifier: GPL-2.0 */
#ifndef BLKTRACE_H
#define BLKTRACE_H

#include <linux/blk-mq.h>
#include <linux/relay.h>
#include <linux/compat.h>
#include <uapi/linux/blktrace_api.h>
#include <linux/list.h>
#include <linux/blk_types.h>

# define blk_trace_ioctl(bdev, cmd, arg)		(-ENOTTY)
# define blk_trace_shutdown(q)				do { } while (0)
# define blk_add_driver_data(rq, data, len)		do {} while (0)
# define blk_trace_setup(q, name, dev, bdev, arg)	(-ENOTTY)
# define blk_trace_startstop(q, start)			(-ENOTTY)
# define blk_add_trace_msg(q, fmt, ...)			do { } while (0)
# define blk_add_cgroup_trace_msg(q, cg, fmt, ...)	do { } while (0)
# define blk_trace_note_message_enabled(q)		(false)

static inline int blk_trace_remove(struct request_queue *q)
{
	return -ENOTTY;
}


void blk_fill_rwbs(char *rwbs, blk_opf_t opf);

static inline sector_t blk_rq_trace_sector(struct request *rq)
{
	/*
	 * Tracing should ignore starting sector for passthrough requests and
	 * requests where starting sector didn't get set.
	 */
	if (blk_rq_is_passthrough(rq) || blk_rq_pos(rq) == (sector_t)-1)
		return 0;
	return blk_rq_pos(rq);
}

static inline unsigned int blk_rq_trace_nr_sectors(struct request *rq)
{
	return blk_rq_is_passthrough(rq) ? 0 : blk_rq_sectors(rq);
}

#endif
