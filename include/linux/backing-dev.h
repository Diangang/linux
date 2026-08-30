/* SPDX-License-Identifier: GPL-2.0 */
/*
 * include/linux/backing-dev.h
 *
 * low-level device information and state which is propagated up through
 * to high-level code.
 */

#ifndef _LINUX_BACKING_DEV_H
#define _LINUX_BACKING_DEV_H

#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/device.h>
#include <linux/writeback.h>
#include <linux/backing-dev-defs.h>
#include <linux/slab.h>

static inline struct backing_dev_info *bdi_get(struct backing_dev_info *bdi)
{
	kref_get(&bdi->refcnt);
	return bdi;
}

struct backing_dev_info *bdi_get_by_id(u64 id);
void bdi_put(struct backing_dev_info *bdi);

__printf(2, 3)
int bdi_register(struct backing_dev_info *bdi, const char *fmt, ...);
__printf(2, 0)
int bdi_register_va(struct backing_dev_info *bdi, const char *fmt,
		    va_list args);
void bdi_set_owner(struct backing_dev_info *bdi, struct device *owner);
void bdi_unregister(struct backing_dev_info *bdi);

struct backing_dev_info *bdi_alloc(int node_id);

void wb_start_background_writeback(struct bdi_writeback *wb);
void wb_workfn(struct work_struct *work);

void wb_wait_for_completion(struct wb_completion *done);

extern spinlock_t bdi_lock;
extern struct list_head bdi_list;

extern struct workqueue_struct *bdi_wq;

static inline bool wb_has_dirty_io(struct bdi_writeback *wb)
{
	return test_bit(WB_has_dirty_io, &wb->state);
}

static inline bool bdi_has_dirty_io(struct backing_dev_info *bdi)
{
	/*
	 * @bdi->tot_write_bandwidth is guaranteed to be > 0 if there are
	 * any dirty wbs.  See wb_update_write_bandwidth().
	 */
	return atomic_long_read(&bdi->tot_write_bandwidth);
}

static inline void wb_stat_mod(struct bdi_writeback *wb,
				 enum wb_stat_item item, s64 amount)
{
	percpu_counter_add_batch(&wb->stat[item], amount, WB_STAT_BATCH);
}

static inline s64 wb_stat(struct bdi_writeback *wb, enum wb_stat_item item)
{
	return percpu_counter_read_positive(&wb->stat[item]);
}

static inline s64 wb_stat_sum(struct bdi_writeback *wb, enum wb_stat_item item)
{
	return percpu_counter_sum_positive(&wb->stat[item]);
}

extern void wb_writeout_inc(struct bdi_writeback *wb);

/*
 * maximal error of a stat counter.
 */
static inline unsigned long wb_stat_error(void)
{
#ifdef CONFIG_SMP
	return nr_cpu_ids * WB_STAT_BATCH;
#else
	return 1;
#endif
}

/* BDI ratio is expressed as part per 1000000 for finer granularity. */
#define BDI_RATIO_SCALE 10000

u64 bdi_get_min_bytes(struct backing_dev_info *bdi);
u64 bdi_get_max_bytes(struct backing_dev_info *bdi);
int bdi_set_min_ratio(struct backing_dev_info *bdi, unsigned int min_ratio);
int bdi_set_max_ratio(struct backing_dev_info *bdi, unsigned int max_ratio);
int bdi_set_min_ratio_no_scale(struct backing_dev_info *bdi, unsigned int min_ratio);
int bdi_set_max_ratio_no_scale(struct backing_dev_info *bdi, unsigned int max_ratio);
int bdi_set_min_bytes(struct backing_dev_info *bdi, u64 min_bytes);
int bdi_set_max_bytes(struct backing_dev_info *bdi, u64 max_bytes);
int bdi_set_strict_limit(struct backing_dev_info *bdi, unsigned int strict_limit);

/*
 * Flags in backing_dev_info::capability
 *
 * BDI_CAP_WRITEBACK:		Supports dirty page writeback, and dirty pages
 *				should contribute to accounting
 * BDI_CAP_STRICTLIMIT:		Keep number of dirty pages below bdi threshold
 */
#define BDI_CAP_WRITEBACK		(1 << 0)
#define BDI_CAP_STRICTLIMIT		(1 << 1)

extern struct backing_dev_info noop_backing_dev_info;

int bdi_init(struct backing_dev_info *bdi);

/**
 * writeback_in_progress - determine whether there is writeback in progress
 * @wb: bdi_writeback of interest
 *
 * Determine whether there is writeback waiting to be handled against a
 * bdi_writeback.
 */
static inline bool writeback_in_progress(struct bdi_writeback *wb)
{
	return test_bit(WB_writeback_running, &wb->state);
}

struct backing_dev_info *inode_to_bdi(struct inode *inode);

static inline bool mapping_can_writeback(struct address_space *mapping)
{
	return inode_to_bdi(mapping->host)->capabilities & BDI_CAP_WRITEBACK;
}

static inline bool inode_cgwb_enabled(struct inode *inode)
{
	return false;
}

static inline struct bdi_writeback *
wb_get_create_current(struct backing_dev_info *bdi, gfp_t gfp)
{
	return &bdi->wb;
}

static inline struct bdi_writeback *inode_to_wb(struct inode *inode)
{
	return &inode_to_bdi(inode)->wb;
}

static inline struct bdi_writeback *inode_to_wb_wbc(
				struct inode *inode,
				struct writeback_control *wbc)
{
	return inode_to_wb(inode);
}


static inline struct bdi_writeback *
unlocked_inode_to_wb_begin(struct inode *inode, struct wb_lock_cookie *cookie)
{
	return inode_to_wb(inode);
}

static inline void unlocked_inode_to_wb_end(struct inode *inode,
					    struct wb_lock_cookie *cookie)
{
}

const char *bdi_dev_name(struct backing_dev_info *bdi);

#endif	/* _LINUX_BACKING_DEV_H */
