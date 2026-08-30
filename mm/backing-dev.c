// SPDX-License-Identifier: GPL-2.0-only

#include <linux/blkdev.h>
#include <linux/wait.h>
#include <linux/rbtree.h>
#include <linux/kthread.h>
#include <linux/backing-dev.h>
#include <linux/freezer.h>
#include <linux/fs.h>
#include <linux/pagemap.h>
#include <linux/mm.h>
#include <linux/sched/mm.h>
#include <linux/sched.h>
#include <linux/module.h>
#include <linux/writeback.h>
#include <linux/device.h>
#include "internal.h"

struct backing_dev_info noop_backing_dev_info;
EXPORT_SYMBOL_GPL(noop_backing_dev_info);

static const char *bdi_unknown_name = "(unknown)";

/*
 * bdi_lock protects bdi_tree and updates to bdi_list. bdi_list has RCU
 * reader side locking.
 */
DEFINE_SPINLOCK(bdi_lock);
static u64 bdi_id_cursor;
static struct rb_root bdi_tree = RB_ROOT;
LIST_HEAD(bdi_list);

/* bdi_wq serves all asynchronous writeback tasks */
struct workqueue_struct *bdi_wq;

static ssize_t read_ahead_kb_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
	struct backing_dev_info *bdi = dev_get_drvdata(dev);
	unsigned long read_ahead_kb;
	ssize_t ret;

	ret = kstrtoul(buf, 10, &read_ahead_kb);
	if (ret < 0)
		return ret;

	bdi->ra_pages = read_ahead_kb >> (PAGE_SHIFT - 10);

	return count;
}

#define BDI_SHOW(name, expr)						\
static ssize_t name##_show(struct device *dev,				\
			   struct device_attribute *attr, char *buf)	\
{									\
	struct backing_dev_info *bdi = dev_get_drvdata(dev);		\
									\
	return sysfs_emit(buf, "%lld\n", (long long)expr);		\
}									\
static DEVICE_ATTR_RW(name);

BDI_SHOW(read_ahead_kb, K(bdi->ra_pages))

static ssize_t min_ratio_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct backing_dev_info *bdi = dev_get_drvdata(dev);
	unsigned int ratio;
	ssize_t ret;

	ret = kstrtouint(buf, 10, &ratio);
	if (ret < 0)
		return ret;

	ret = bdi_set_min_ratio(bdi, ratio);
	if (!ret)
		ret = count;

	return ret;
}
BDI_SHOW(min_ratio, bdi->min_ratio / BDI_RATIO_SCALE)

static ssize_t min_ratio_fine_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct backing_dev_info *bdi = dev_get_drvdata(dev);
	unsigned int ratio;
	ssize_t ret;

	ret = kstrtouint(buf, 10, &ratio);
	if (ret < 0)
		return ret;

	ret = bdi_set_min_ratio_no_scale(bdi, ratio);
	if (!ret)
		ret = count;

	return ret;
}
BDI_SHOW(min_ratio_fine, bdi->min_ratio)

static ssize_t max_ratio_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct backing_dev_info *bdi = dev_get_drvdata(dev);
	unsigned int ratio;
	ssize_t ret;

	ret = kstrtouint(buf, 10, &ratio);
	if (ret < 0)
		return ret;

	ret = bdi_set_max_ratio(bdi, ratio);
	if (!ret)
		ret = count;

	return ret;
}
BDI_SHOW(max_ratio, bdi->max_ratio / BDI_RATIO_SCALE)

static ssize_t max_ratio_fine_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct backing_dev_info *bdi = dev_get_drvdata(dev);
	unsigned int ratio;
	ssize_t ret;

	ret = kstrtouint(buf, 10, &ratio);
	if (ret < 0)
		return ret;

	ret = bdi_set_max_ratio_no_scale(bdi, ratio);
	if (!ret)
		ret = count;

	return ret;
}
BDI_SHOW(max_ratio_fine, bdi->max_ratio)

static ssize_t min_bytes_show(struct device *dev,
			      struct device_attribute *attr,
			      char *buf)
{
	struct backing_dev_info *bdi = dev_get_drvdata(dev);

	return sysfs_emit(buf, "%llu\n", bdi_get_min_bytes(bdi));
}

static ssize_t min_bytes_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct backing_dev_info *bdi = dev_get_drvdata(dev);
	u64 bytes;
	ssize_t ret;

	ret = kstrtoull(buf, 10, &bytes);
	if (ret < 0)
		return ret;

	ret = bdi_set_min_bytes(bdi, bytes);
	if (!ret)
		ret = count;

	return ret;
}
static DEVICE_ATTR_RW(min_bytes);

static ssize_t max_bytes_show(struct device *dev,
			      struct device_attribute *attr,
			      char *buf)
{
	struct backing_dev_info *bdi = dev_get_drvdata(dev);

	return sysfs_emit(buf, "%llu\n", bdi_get_max_bytes(bdi));
}

static ssize_t max_bytes_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct backing_dev_info *bdi = dev_get_drvdata(dev);
	u64 bytes;
	ssize_t ret;

	ret = kstrtoull(buf, 10, &bytes);
	if (ret < 0)
		return ret;

	ret = bdi_set_max_bytes(bdi, bytes);
	if (!ret)
		ret = count;

	return ret;
}
static DEVICE_ATTR_RW(max_bytes);

static ssize_t stable_pages_required_show(struct device *dev,
					  struct device_attribute *attr,
					  char *buf)
{
	dev_warn_once(dev,
		"the stable_pages_required attribute has been removed. Use the stable_writes queue attribute instead.\n");
	return sysfs_emit(buf, "%d\n", 0);
}
static DEVICE_ATTR_RO(stable_pages_required);

static ssize_t strict_limit_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct backing_dev_info *bdi = dev_get_drvdata(dev);
	unsigned int strict_limit;
	ssize_t ret;

	ret = kstrtouint(buf, 10, &strict_limit);
	if (ret < 0)
		return ret;

	ret = bdi_set_strict_limit(bdi, strict_limit);
	if (!ret)
		ret = count;

	return ret;
}

static ssize_t strict_limit_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct backing_dev_info *bdi = dev_get_drvdata(dev);

	return sysfs_emit(buf, "%d\n",
			!!(bdi->capabilities & BDI_CAP_STRICTLIMIT));
}
static DEVICE_ATTR_RW(strict_limit);

static struct attribute *bdi_dev_attrs[] = {
	&dev_attr_read_ahead_kb.attr,
	&dev_attr_min_ratio.attr,
	&dev_attr_min_ratio_fine.attr,
	&dev_attr_max_ratio.attr,
	&dev_attr_max_ratio_fine.attr,
	&dev_attr_min_bytes.attr,
	&dev_attr_max_bytes.attr,
	&dev_attr_stable_pages_required.attr,
	&dev_attr_strict_limit.attr,
	NULL,
};
ATTRIBUTE_GROUPS(bdi_dev);

static const struct class bdi_class = {
	.name		= "bdi",
	.dev_groups	= bdi_dev_groups,
};

static __init int bdi_class_init(void)
{
	return class_register(&bdi_class);
}
postcore_initcall(bdi_class_init);

static int __init default_bdi_init(void)
{
	bdi_wq = alloc_workqueue("writeback", WQ_MEM_RECLAIM | WQ_UNBOUND |
				 WQ_SYSFS, 0);
	if (!bdi_wq)
		return -ENOMEM;
	return 0;
}
subsys_initcall(default_bdi_init);

static void wb_update_bandwidth_workfn(struct work_struct *work)
{
	struct bdi_writeback *wb = container_of(to_delayed_work(work),
						struct bdi_writeback, bw_dwork);

	wb_update_bandwidth(wb);
}

/*
 * Initial write bandwidth: 100 MB/s
 */
#define INIT_BW		MB_TO_PAGES(100)

static int wb_init(struct bdi_writeback *wb, struct backing_dev_info *bdi,
		   gfp_t gfp)
{
	int err;

	memset(wb, 0, sizeof(*wb));

	wb->bdi = bdi;
	wb->last_old_flush = jiffies;
	INIT_LIST_HEAD(&wb->b_dirty);
	INIT_LIST_HEAD(&wb->b_io);
	INIT_LIST_HEAD(&wb->b_more_io);
	INIT_LIST_HEAD(&wb->b_dirty_time);
	spin_lock_init(&wb->list_lock);

	atomic_set(&wb->writeback_inodes, 0);
	wb->bw_time_stamp = jiffies;
	wb->balanced_dirty_ratelimit = INIT_BW;
	wb->dirty_ratelimit = INIT_BW;
	wb->write_bandwidth = INIT_BW;
	wb->avg_write_bandwidth = INIT_BW;

	spin_lock_init(&wb->work_lock);
	INIT_LIST_HEAD(&wb->work_list);
	INIT_DELAYED_WORK(&wb->dwork, wb_workfn);
	INIT_DELAYED_WORK(&wb->bw_dwork, wb_update_bandwidth_workfn);

	err = fprop_local_init_percpu(&wb->completions, gfp);
	if (err)
		return err;

	err = percpu_counter_init_many(wb->stat, 0, gfp, NR_WB_STAT_ITEMS);
	if (err)
		fprop_local_destroy_percpu(&wb->completions);

	return err;
}

/*
 * Remove bdi from the global list and shutdown any threads we have running
 */
static void wb_shutdown(struct bdi_writeback *wb)
{
	/* Make sure nobody queues further work */
	spin_lock_irq(&wb->work_lock);
	if (!test_and_clear_bit(WB_registered, &wb->state)) {
		spin_unlock_irq(&wb->work_lock);
		return;
	}
	spin_unlock_irq(&wb->work_lock);

	list_del_rcu(&wb->bdi_node);
	/*
	 * Drain work list and shutdown the delayed_work.  !WB_registered
	 * tells wb_workfn() that @wb is dying and its work_list needs to
	 * be drained no matter what.
	 */
	mod_delayed_work(bdi_wq, &wb->dwork, 0);
	flush_delayed_work(&wb->dwork);
	WARN_ON(!list_empty(&wb->work_list));
	flush_delayed_work(&wb->bw_dwork);
}

static void wb_exit(struct bdi_writeback *wb)
{
	WARN_ON(delayed_work_pending(&wb->dwork));
	percpu_counter_destroy_many(wb->stat, NR_WB_STAT_ITEMS);
	fprop_local_destroy_percpu(&wb->completions);
}


int bdi_init(struct backing_dev_info *bdi)
{
	bdi->dev = NULL;

	kref_init(&bdi->refcnt);
	bdi->min_ratio = 0;
	bdi->max_ratio = 100 * BDI_RATIO_SCALE;
	bdi->max_prop_frac = FPROP_FRAC_BASE;
	INIT_LIST_HEAD(&bdi->bdi_list);
	INIT_LIST_HEAD(&bdi->wb_list);
	init_waitqueue_head(&bdi->wb_waitq);
	bdi->last_bdp_sleep = jiffies;

	return wb_init(&bdi->wb, bdi, GFP_KERNEL);
}

struct backing_dev_info *bdi_alloc(int node_id)
{
	struct backing_dev_info *bdi;

	bdi = kzalloc_node(sizeof(*bdi), GFP_KERNEL, node_id);
	if (!bdi)
		return NULL;

	if (bdi_init(bdi)) {
		kfree(bdi);
		return NULL;
	}
	bdi->capabilities = BDI_CAP_WRITEBACK;
	bdi->ra_pages = VM_READAHEAD_PAGES;
	bdi->io_pages = VM_READAHEAD_PAGES;
	return bdi;
}
EXPORT_SYMBOL(bdi_alloc);

static struct rb_node **bdi_lookup_rb_node(u64 id, struct rb_node **parentp)
{
	struct rb_node **p = &bdi_tree.rb_node;
	struct rb_node *parent = NULL;
	struct backing_dev_info *bdi;

	lockdep_assert_held(&bdi_lock);

	while (*p) {
		parent = *p;
		bdi = rb_entry(parent, struct backing_dev_info, rb_node);

		if (bdi->id > id)
			p = &(*p)->rb_left;
		else if (bdi->id < id)
			p = &(*p)->rb_right;
		else
			break;
	}

	if (parentp)
		*parentp = parent;
	return p;
}

/**
 * bdi_get_by_id - lookup and get bdi from its id
 * @id: bdi id to lookup
 *
 * Find bdi matching @id and get it.  Returns NULL if the matching bdi
 * doesn't exist or is already unregistered.
 */
struct backing_dev_info *bdi_get_by_id(u64 id)
{
	struct backing_dev_info *bdi = NULL;
	struct rb_node **p;

	spin_lock_bh(&bdi_lock);
	p = bdi_lookup_rb_node(id, NULL);
	if (*p) {
		bdi = rb_entry(*p, struct backing_dev_info, rb_node);
		bdi_get(bdi);
	}
	spin_unlock_bh(&bdi_lock);

	return bdi;
}

int bdi_register_va(struct backing_dev_info *bdi, const char *fmt, va_list args)
{
	struct device *dev;
	struct rb_node *parent, **p;

	if (bdi->dev)	/* The driver needs to use separate queues per device */
		return 0;

	vsnprintf(bdi->dev_name, sizeof(bdi->dev_name), fmt, args);
	dev = device_create(&bdi_class, NULL, MKDEV(0, 0), bdi, bdi->dev_name);
	if (IS_ERR(dev))
		return PTR_ERR(dev);

	list_add_tail_rcu(&bdi->wb.bdi_node, &bdi->wb_list);
	bdi->dev = dev;

	set_bit(WB_registered, &bdi->wb.state);

	spin_lock_bh(&bdi_lock);

	bdi->id = ++bdi_id_cursor;

	p = bdi_lookup_rb_node(bdi->id, &parent);
	rb_link_node(&bdi->rb_node, parent, p);
	rb_insert_color(&bdi->rb_node, &bdi_tree);

	list_add_tail_rcu(&bdi->bdi_list, &bdi_list);

	spin_unlock_bh(&bdi_lock);

	return 0;
}

int bdi_register(struct backing_dev_info *bdi, const char *fmt, ...)
{
	va_list args;
	int ret;

	va_start(args, fmt);
	ret = bdi_register_va(bdi, fmt, args);
	va_end(args);
	return ret;
}
EXPORT_SYMBOL(bdi_register);

void bdi_set_owner(struct backing_dev_info *bdi, struct device *owner)
{
	WARN_ON_ONCE(bdi->owner);
	bdi->owner = owner;
	get_device(owner);
}

/*
 * Remove bdi from bdi_list, and ensure that it is no longer visible
 */
static void bdi_remove_from_list(struct backing_dev_info *bdi)
{
	spin_lock_bh(&bdi_lock);
	rb_erase(&bdi->rb_node, &bdi_tree);
	list_del_rcu(&bdi->bdi_list);
	spin_unlock_bh(&bdi_lock);

	synchronize_rcu_expedited();
}

void bdi_unregister(struct backing_dev_info *bdi)
{
	/* make sure nobody finds us on the bdi_list anymore */
	bdi_remove_from_list(bdi);
	wb_shutdown(&bdi->wb);

	/*
	 * If this BDI's min ratio has been set, use bdi_set_min_ratio() to
	 * update the global bdi_min_ratio.
	 */
	if (bdi->min_ratio)
		bdi_set_min_ratio(bdi, 0);

	if (bdi->dev) {
		device_unregister(bdi->dev);
		bdi->dev = NULL;
	}

	if (bdi->owner) {
		put_device(bdi->owner);
		bdi->owner = NULL;
	}
}
EXPORT_SYMBOL(bdi_unregister);

static void release_bdi(struct kref *ref)
{
	struct backing_dev_info *bdi =
			container_of(ref, struct backing_dev_info, refcnt);

	WARN_ON_ONCE(test_bit(WB_registered, &bdi->wb.state));
	WARN_ON_ONCE(bdi->dev);
	wb_exit(&bdi->wb);
	kfree(bdi);
}

void bdi_put(struct backing_dev_info *bdi)
{
	kref_put(&bdi->refcnt, release_bdi);
}
EXPORT_SYMBOL(bdi_put);

struct backing_dev_info *inode_to_bdi(struct inode *inode)
{
	struct super_block *sb;

	if (!inode)
		return &noop_backing_dev_info;

	sb = inode->i_sb;
#ifdef CONFIG_BLOCK
	if (sb_is_blkdev_sb(sb))
		return I_BDEV(inode)->bd_disk->bdi;
#endif
	return sb->s_bdi;
}
EXPORT_SYMBOL(inode_to_bdi);

const char *bdi_dev_name(struct backing_dev_info *bdi)
{
	if (!bdi || !bdi->dev)
		return bdi_unknown_name;
	return bdi->dev_name;
}
EXPORT_SYMBOL_GPL(bdi_dev_name);
