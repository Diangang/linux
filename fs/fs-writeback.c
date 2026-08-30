// SPDX-License-Identifier: GPL-2.0-only
/*
 * fs/fs-writeback.c
 *
 * Copyright (C) 2002, Linus Torvalds.
 *
 * Contains all the functions related to writing back and waiting
 * upon dirty inodes against superblocks, and writing back dirty
 * pages against inodes.  ie: data writeback.  Writeout of the
 * inode itself is not handled here.
 *
 * 10Apr2002	Andrew Morton
 *		Split out of fs/inode.c
 *		Additions for address_space-based writeback
 */

#include <linux/sched/sysctl.h>
#include <linux/kernel.h>
#include <linux/export.h>
#include <linux/spinlock.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/pagemap.h>
#include <linux/kthread.h>
#include <linux/writeback.h>
#include <linux/blkdev.h>
#include <linux/backing-dev.h>
#include <linux/tracepoint.h>
#include <linux/device.h>
#include "internal.h"

/*
 * Passed into wb_writeback(), essentially a subset of writeback_control
 */
struct wb_writeback_work {
	long nr_pages;
	struct super_block *sb;
	enum writeback_sync_modes sync_mode;
	unsigned int tagged_writepages:1;
	unsigned int for_kupdate:1;
	unsigned int range_cyclic:1;
	unsigned int for_background:1;
	unsigned int for_sync:1;	/* sync(2) WB_SYNC_ALL writeback */
	unsigned int auto_free:1;	/* free on completion */
	enum wb_reason reason;		/* why was writeback initiated? */

	struct list_head list;		/* pending work list */
	struct wb_completion *done;	/* set if the caller waits */
};

/*
 * If an inode is constantly having its pages dirtied, but then the
 * updates stop dirtytime_expire_interval seconds in the past, it's
 * possible for the worst case time between when an inode has its
 * timestamps updated and when they finally get written out to be two
 * dirtytime_expire_intervals.  We set the default to 12 hours (in
 * seconds), which means most of the time inodes will have their
 * timestamps written to disk after 12 hours, but in the worst case a
 * few inodes might not their timestamps updated for 24 hours.
 */
static unsigned int dirtytime_expire_interval = 12 * 60 * 60;

static inline struct inode *wb_inode(struct list_head *head)
{
	return list_entry(head, struct inode, i_io_list);
}

/*
 * Include the creation of the trace points after defining the
 * wb_writeback_work structure and inline functions so that the definition
 * remains local to this file.
 */
static bool wb_io_lists_populated(struct bdi_writeback *wb)
{
	if (wb_has_dirty_io(wb)) {
		return false;
	} else {
		set_bit(WB_has_dirty_io, &wb->state);
		WARN_ON_ONCE(!wb->avg_write_bandwidth);
		atomic_long_add(wb->avg_write_bandwidth,
				&wb->bdi->tot_write_bandwidth);
		return true;
	}
}

static void wb_io_lists_depopulated(struct bdi_writeback *wb)
{
	if (wb_has_dirty_io(wb) && list_empty(&wb->b_dirty) &&
	    list_empty(&wb->b_io) && list_empty(&wb->b_more_io)) {
		clear_bit(WB_has_dirty_io, &wb->state);
		WARN_ON_ONCE(atomic_long_sub_return(wb->avg_write_bandwidth,
					&wb->bdi->tot_write_bandwidth) < 0);
	}
}

/**
 * inode_io_list_move_locked - move an inode onto a bdi_writeback IO list
 * @inode: inode to be moved
 * @wb: target bdi_writeback
 * @head: one of @wb->b_{dirty|io|more_io|dirty_time}
 *
 * Move @inode->i_io_list to @list of @wb and set %WB_has_dirty_io.
 * Returns %true if @inode is the first occupant of the !dirty_time IO
 * lists; otherwise, %false.
 */
static bool inode_io_list_move_locked(struct inode *inode,
				      struct bdi_writeback *wb,
				      struct list_head *head)
{
	assert_spin_locked(&wb->list_lock);
	assert_spin_locked(&inode->i_lock);
	WARN_ON_ONCE(inode_state_read(inode) & I_FREEING);

	list_move(&inode->i_io_list, head);

	/* dirty_time doesn't count as dirty_io until expiration */
	if (head != &wb->b_dirty_time)
		return wb_io_lists_populated(wb);

	wb_io_lists_depopulated(wb);
	return false;
}

static void wb_wakeup(struct bdi_writeback *wb)
{
	spin_lock_irq(&wb->work_lock);
	if (test_bit(WB_registered, &wb->state))
		mod_delayed_work(bdi_wq, &wb->dwork, 0);
	spin_unlock_irq(&wb->work_lock);
}

/*
 * This function is used when the first inode for this wb is marked dirty. It
 * wakes-up the corresponding bdi thread which should then take care of the
 * periodic background write-out of dirty inodes. Since the write-out would
 * starts only 'dirty_writeback_interval' centisecs from now anyway, we just
 * set up a timer which wakes the bdi thread up later.
 *
 * Note, we wouldn't bother setting up the timer, but this function is on the
 * fast-path (used by '__mark_inode_dirty()'), so we save few context switches
 * by delaying the wake-up.
 *
 * We have to be careful not to postpone flush work if it is scheduled for
 * earlier. Thus we use queue_delayed_work().
 */
static void wb_wakeup_delayed(struct bdi_writeback *wb)
{
	unsigned long timeout;

	timeout = msecs_to_jiffies(dirty_writeback_interval * 10);
	spin_lock_irq(&wb->work_lock);
	if (test_bit(WB_registered, &wb->state))
		queue_delayed_work(bdi_wq, &wb->dwork, timeout);
	spin_unlock_irq(&wb->work_lock);
}

static void finish_writeback_work(struct wb_writeback_work *work)
{
	struct wb_completion *done = work->done;

	if (work->auto_free)
		kfree(work);
	if (done) {
		wait_queue_head_t *waitq = done->waitq;

		/* @done can't be accessed after the following dec */
		if (atomic_dec_and_test(&done->cnt))
			wake_up_all(waitq);
	}
}

static void wb_queue_work(struct bdi_writeback *wb,
			  struct wb_writeback_work *work)
{

	if (work->done)
		atomic_inc(&work->done->cnt);

	spin_lock_irq(&wb->work_lock);

	if (test_bit(WB_registered, &wb->state)) {
		list_add_tail(&work->list, &wb->work_list);
		mod_delayed_work(bdi_wq, &wb->dwork, 0);
	} else
		finish_writeback_work(work);

	spin_unlock_irq(&wb->work_lock);
}

static bool wb_wait_for_completion_cb(struct wb_completion *done)
{
	unsigned long timeout = sysctl_hung_task_timeout_secs;
	unsigned long waited_secs = (jiffies - done->wait_start) / HZ;

	done->progress_stamp = jiffies;
	if (timeout && (waited_secs > timeout))
		pr_info("INFO: The task %s:%d has been waiting for writeback "
			"completion for more than %lu seconds.",
			current->comm, current->pid, waited_secs);

	return !atomic_read(&done->cnt);
}

/**
 * wb_wait_for_completion - wait for completion of bdi_writeback_works
 * @done: target wb_completion
 *
 * Wait for one or more work items issued to @bdi with their ->done field
 * set to @done, which should have been initialized with
 * DEFINE_WB_COMPLETION().  This function returns after all such work items
 * are completed.  Work items which are waited upon aren't freed
 * automatically on completion.
 */
void wb_wait_for_completion(struct wb_completion *done)
{
	done->wait_start = jiffies;
	atomic_dec(&done->cnt);		/* put down the initial count */
	wait_event(*done->waitq, wb_wait_for_completion_cb(done));
}


static void bdi_down_write_wb_switch_rwsem(struct backing_dev_info *bdi) { }
static void bdi_up_write_wb_switch_rwsem(struct backing_dev_info *bdi) { }

static void inode_cgwb_move_to_attached(struct inode *inode,
					struct bdi_writeback *wb)
{
	assert_spin_locked(&wb->list_lock);
	assert_spin_locked(&inode->i_lock);
	WARN_ON_ONCE(inode_state_read(inode) & I_FREEING);

	inode_state_clear(inode, I_SYNC_QUEUED);
	list_del_init(&inode->i_io_list);
	wb_io_lists_depopulated(wb);
}

static struct bdi_writeback *
locked_inode_to_wb_and_lock_list(struct inode *inode)
	__releases(&inode->i_lock)
	__acquires(&wb->list_lock)
{
	struct bdi_writeback *wb = inode_to_wb(inode);

	spin_unlock(&inode->i_lock);
	spin_lock(&wb->list_lock);
	return wb;
}

static struct bdi_writeback *inode_to_wb_and_lock_list(struct inode *inode)
	__acquires(&wb->list_lock)
{
	struct bdi_writeback *wb = inode_to_wb(inode);

	spin_lock(&wb->list_lock);
	return wb;
}

static void bdi_split_work_to_wbs(struct backing_dev_info *bdi,
				  struct wb_writeback_work *base_work,
				  bool skip_if_busy)
{
	might_sleep();

	if (!skip_if_busy || !writeback_in_progress(&bdi->wb)) {
		base_work->auto_free = 0;
		wb_queue_work(&bdi->wb, base_work);
	}
}

static inline void wbc_attach_and_unlock_inode(struct writeback_control *wbc,
					       struct inode *inode)
	__releases(&inode->i_lock)
{
	spin_unlock(&inode->i_lock);
}


/*
 * Add in the number of potentially dirty inodes, because each inode
 * write can dirty pagecache in the underlying blockdev.
 */
static unsigned long get_nr_dirty_pages(void)
{
	return global_node_page_state(NR_FILE_DIRTY) +
		get_nr_dirty_inodes();
}

static void wb_start_writeback(struct bdi_writeback *wb, enum wb_reason reason)
{
	if (!wb_has_dirty_io(wb))
		return;

	/*
	 * All callers of this function want to start writeback of all
	 * dirty pages. Places like vmscan can call this at a very
	 * high frequency, causing pointless allocations of tons of
	 * work items and keeping the flusher threads busy retrieving
	 * that work. Ensure that we only allow one of them pending and
	 * inflight at the time.
	 */
	if (test_bit(WB_start_all, &wb->state) ||
	    test_and_set_bit(WB_start_all, &wb->state))
		return;

	wb->start_all_reason = reason;
	wb_wakeup(wb);
}

/**
 * wb_start_background_writeback - start background writeback
 * @wb: bdi_writback to write from
 *
 * Description:
 *   This makes sure WB_SYNC_NONE background writeback happens. When
 *   this function returns, it is only guaranteed that for given wb
 *   some IO is happening if we are over background dirty threshold.
 *   Caller need not hold sb s_umount semaphore.
 */
void wb_start_background_writeback(struct bdi_writeback *wb)
{
	/*
	 * We just wake up the flusher thread. It will perform background
	 * writeback as soon as there is no other work to do.
	 */
	wb_wakeup(wb);
}

/*
 * Remove the inode from the writeback list it is on.
 */
void inode_io_list_del(struct inode *inode)
{
	struct bdi_writeback *wb;

	/*
	 * FIXME: ext4 can call here from ext4_evict_inode() after evict() already
	 * unlinked the inode.
	 */
	if (list_empty_careful(&inode->i_io_list))
		return;

	wb = inode_to_wb_and_lock_list(inode);
	spin_lock(&inode->i_lock);

	inode_state_clear(inode, I_SYNC_QUEUED);
	list_del_init(&inode->i_io_list);
	wb_io_lists_depopulated(wb);

	spin_unlock(&inode->i_lock);
	spin_unlock(&wb->list_lock);
}
EXPORT_SYMBOL(inode_io_list_del);

/*
 * mark an inode as under writeback on the sb
 */
void sb_mark_inode_writeback(struct inode *inode)
{
	struct super_block *sb = inode->i_sb;
	unsigned long flags;

	if (list_empty(&inode->i_wb_list)) {
		spin_lock_irqsave(&sb->s_inode_wblist_lock, flags);
		if (list_empty(&inode->i_wb_list)) {
			list_add_tail(&inode->i_wb_list, &sb->s_inodes_wb);
		}
		spin_unlock_irqrestore(&sb->s_inode_wblist_lock, flags);
	}
}

/*
 * clear an inode as under writeback on the sb
 */
void sb_clear_inode_writeback(struct inode *inode)
{
	struct super_block *sb = inode->i_sb;
	unsigned long flags;

	if (!list_empty(&inode->i_wb_list)) {
		spin_lock_irqsave(&sb->s_inode_wblist_lock, flags);
		if (!list_empty(&inode->i_wb_list)) {
			list_del_init(&inode->i_wb_list);
		}
		spin_unlock_irqrestore(&sb->s_inode_wblist_lock, flags);
	}
}

/*
 * Redirty an inode: set its when-it-was dirtied timestamp and move it to the
 * furthest end of its superblock's dirty-inode list.
 *
 * Before stamping the inode's ->dirtied_when, we check to see whether it is
 * already the most-recently-dirtied inode on the b_dirty list.  If that is
 * the case then the inode must have been redirtied while it was being written
 * out and we don't reset its dirtied_when.
 */
static void redirty_tail_locked(struct inode *inode, struct bdi_writeback *wb)
{
	assert_spin_locked(&inode->i_lock);

	inode_state_clear(inode, I_SYNC_QUEUED);
	/*
	 * When the inode is being freed just don't bother with dirty list
	 * tracking. Flush worker will ignore this inode anyway and it will
	 * trigger assertions in inode_io_list_move_locked().
	 */
	if (inode_state_read(inode) & I_FREEING) {
		list_del_init(&inode->i_io_list);
		wb_io_lists_depopulated(wb);
		return;
	}
	if (!list_empty(&wb->b_dirty)) {
		struct inode *tail;

		tail = wb_inode(wb->b_dirty.next);
		if (time_before(inode->dirtied_when, tail->dirtied_when))
			inode->dirtied_when = jiffies;
	}
	inode_io_list_move_locked(inode, wb, &wb->b_dirty);
}

static void redirty_tail(struct inode *inode, struct bdi_writeback *wb)
{
	spin_lock(&inode->i_lock);
	redirty_tail_locked(inode, wb);
	spin_unlock(&inode->i_lock);
}

/*
 * requeue inode for re-scanning after bdi->b_io list is exhausted.
 */
static void requeue_io(struct inode *inode, struct bdi_writeback *wb)
{
	inode_io_list_move_locked(inode, wb, &wb->b_more_io);
}

static void inode_sync_complete(struct inode *inode)
{
	assert_spin_locked(&inode->i_lock);

	inode_state_clear(inode, I_SYNC);
	/* If inode is clean an unused, put it into LRU now... */
	inode_lru_list_add(inode);
	/* Called with inode->i_lock which ensures memory ordering. */
	inode_wake_up_bit(inode, __I_SYNC);
}

static bool inode_dirtied_after(struct inode *inode, unsigned long t)
{
	bool ret = time_after(inode->dirtied_when, t);
#ifndef CONFIG_64BIT
	/*
	 * For inodes being constantly redirtied, dirtied_when can get stuck.
	 * It _appears_ to be in the future, but is actually in distant past.
	 * This test is necessary to prevent such wrapped-around relative times
	 * from permanently stopping the whole bdi writeback.
	 */
	ret = ret && time_before_eq(inode->dirtied_when, jiffies);
#endif
	return ret;
}

/*
 * Move expired (dirtied before dirtied_before) dirty inodes from
 * @delaying_queue to @dispatch_queue.
 */
static int move_expired_inodes(struct list_head *delaying_queue,
			       struct list_head *dispatch_queue,
			       unsigned long dirtied_before)
{
	LIST_HEAD(tmp);
	struct list_head *pos, *node;
	struct super_block *sb = NULL;
	struct inode *inode;
	int do_sb_sort = 0;
	int moved = 0;

	while (!list_empty(delaying_queue)) {
		inode = wb_inode(delaying_queue->prev);
		if (inode_dirtied_after(inode, dirtied_before))
			break;
		spin_lock(&inode->i_lock);
		list_move(&inode->i_io_list, &tmp);
		moved++;
		inode_state_set(inode, I_SYNC_QUEUED);
		spin_unlock(&inode->i_lock);
		if (sb_is_blkdev_sb(inode->i_sb))
			continue;
		if (sb && sb != inode->i_sb)
			do_sb_sort = 1;
		sb = inode->i_sb;
	}

	/* just one sb in list, splice to dispatch_queue and we're done */
	if (!do_sb_sort) {
		list_splice(&tmp, dispatch_queue);
		goto out;
	}

	/*
	 * Although inode's i_io_list is moved from 'tmp' to 'dispatch_queue',
	 * we don't take inode->i_lock here because it is just a pointless overhead.
	 * Inode is already marked as I_SYNC_QUEUED so writeback list handling is
	 * fully under our control.
	 */
	while (!list_empty(&tmp)) {
		sb = wb_inode(tmp.prev)->i_sb;
		list_for_each_prev_safe(pos, node, &tmp) {
			inode = wb_inode(pos);
			if (inode->i_sb == sb)
				list_move(&inode->i_io_list, dispatch_queue);
		}
	}
out:
	return moved;
}

/*
 * Queue all expired dirty inodes for io, eldest first.
 * Before
 *         newly dirtied     b_dirty    b_io    b_more_io
 *         =============>    gf         edc     BA
 * After
 *         newly dirtied     b_dirty    b_io    b_more_io
 *         =============>    g          fBAedc
 *                                           |
 *                                           +--> dequeue for IO
 */
static void queue_io(struct bdi_writeback *wb, struct wb_writeback_work *work,
		     unsigned long dirtied_before)
{
	int moved;
	unsigned long time_expire_jif = dirtied_before;

	assert_spin_locked(&wb->list_lock);
	list_splice_init(&wb->b_more_io, &wb->b_io);
	moved = move_expired_inodes(&wb->b_dirty, &wb->b_io, dirtied_before);
	if (!work->for_sync)
		time_expire_jif = jiffies - dirtytime_expire_interval * HZ;
	moved += move_expired_inodes(&wb->b_dirty_time, &wb->b_io,
				     time_expire_jif);
	if (moved)
		wb_io_lists_populated(wb);
}

static int write_inode(struct inode *inode, struct writeback_control *wbc)
{
	int ret;

	if (inode->i_sb->s_op->write_inode && !is_bad_inode(inode)) {
		ret = inode->i_sb->s_op->write_inode(inode, wbc);
		return ret;
	}
	return 0;
}

/*
 * Wait for writeback on an inode to complete. Called with i_lock held.
 * Caller must make sure inode cannot go away when we drop i_lock.
 */
void inode_wait_for_writeback(struct inode *inode)
{
	struct wait_bit_queue_entry wqe;
	struct wait_queue_head *wq_head;

	assert_spin_locked(&inode->i_lock);

	if (!(inode_state_read(inode) & I_SYNC))
		return;

	wq_head = inode_bit_waitqueue(&wqe, inode, __I_SYNC);
	for (;;) {
		prepare_to_wait_event(wq_head, &wqe.wq_entry, TASK_UNINTERRUPTIBLE);
		/* Checking I_SYNC with inode->i_lock guarantees memory ordering. */
		if (!(inode_state_read(inode) & I_SYNC))
			break;
		spin_unlock(&inode->i_lock);
		schedule();
		spin_lock(&inode->i_lock);
	}
	finish_wait(wq_head, &wqe.wq_entry);
}

/*
 * Sleep until I_SYNC is cleared. This function must be called with i_lock
 * held and drops it. It is aimed for callers not holding any inode reference
 * so once i_lock is dropped, inode can go away.
 */
static void inode_sleep_on_writeback(struct inode *inode)
	__releases(inode->i_lock)
{
	struct wait_bit_queue_entry wqe;
	struct wait_queue_head *wq_head;
	bool sleep;

	assert_spin_locked(&inode->i_lock);

	wq_head = inode_bit_waitqueue(&wqe, inode, __I_SYNC);
	prepare_to_wait_event(wq_head, &wqe.wq_entry, TASK_UNINTERRUPTIBLE);
	/* Checking I_SYNC with inode->i_lock guarantees memory ordering. */
	sleep = !!(inode_state_read(inode) & I_SYNC);
	spin_unlock(&inode->i_lock);
	if (sleep)
		schedule();
	finish_wait(wq_head, &wqe.wq_entry);
}

/*
 * Find proper writeback list for the inode depending on its current state and
 * possibly also change of its state while we were doing writeback.  Here we
 * handle things such as livelock prevention or fairness of writeback among
 * inodes. This function can be called only by flusher thread - noone else
 * processes all inodes in writeback lists and requeueing inodes behind flusher
 * thread's back can have unexpected consequences.
 */
static void requeue_inode(struct inode *inode, struct bdi_writeback *wb,
			  struct writeback_control *wbc,
			  unsigned long dirtied_before)
{
	if (inode_state_read(inode) & I_FREEING)
		return;

	/*
	 * Sync livelock prevention. Each inode is tagged and synced in one
	 * shot. If still dirty, it will be redirty_tail()'ed below.  Update
	 * the dirty time to prevent enqueue and sync it again.
	 */
	if ((inode_state_read(inode) & I_DIRTY) &&
	    (wbc->sync_mode == WB_SYNC_ALL || wbc->tagged_writepages))
		inode->dirtied_when = jiffies;

	if (wbc->pages_skipped) {
		/*
		 * Writeback is not making progress due to locked buffers.
		 * Skip this inode for now. Although having skipped pages
		 * is odd for clean inodes, it can happen for some
		 * filesystems so handle that gracefully.
		 */
		if (inode_state_read(inode) & I_DIRTY_ALL)
			redirty_tail_locked(inode, wb);
		else
			inode_cgwb_move_to_attached(inode, wb);
		return;
	}

	if (mapping_tagged(inode->i_mapping, PAGECACHE_TAG_DIRTY)) {
		/*
		 * We didn't write back all the pages.  nfs_writepages()
		 * sometimes bales out without doing anything.
		 */
		if (wbc->nr_to_write <= 0 &&
		    !inode_dirtied_after(inode, dirtied_before)) {
			/* Slice used up. Queue for next turn. */
			requeue_io(inode, wb);
		} else {
			/*
			 * Writeback blocked by something other than
			 * congestion. Delay the inode for some time to
			 * avoid spinning on the CPU (100% iowait)
			 * retrying writeback of the dirty page/inode
			 * that cannot be performed immediately.
			 */
			redirty_tail_locked(inode, wb);
		}
	} else if (inode_state_read(inode) & I_DIRTY) {
		/*
		 * Filesystems can dirty the inode during writeback operations,
		 * such as delayed allocation during submission or metadata
		 * updates after data IO completion.
		 */
		redirty_tail_locked(inode, wb);
	} else if (inode_state_read(inode) & I_DIRTY_TIME) {
		inode->dirtied_when = jiffies;
		inode_io_list_move_locked(inode, wb, &wb->b_dirty_time);
		inode_state_clear(inode, I_SYNC_QUEUED);
	} else {
		/* The inode is clean. Remove from writeback lists. */
		inode_cgwb_move_to_attached(inode, wb);
	}
}

static bool __sync_lazytime(struct inode *inode)
{
	spin_lock(&inode->i_lock);
	if (!(inode_state_read(inode) & I_DIRTY_TIME)) {
		spin_unlock(&inode->i_lock);
		return false;
	}
	inode_state_clear(inode, I_DIRTY_TIME);
	spin_unlock(&inode->i_lock);
	inode->i_op->sync_lazytime(inode);
	return true;
}

bool sync_lazytime(struct inode *inode)
{
	if (!(inode_state_read_once(inode) & I_DIRTY_TIME))
		return false;

	if (inode->i_op->sync_lazytime)
		return __sync_lazytime(inode);
	mark_inode_dirty_sync(inode);
	return true;
}

/*
 * Write out an inode and its dirty pages (or some of its dirty pages, depending
 * on @wbc->nr_to_write), and clear the relevant dirty flags from i_state.
 *
 * This doesn't remove the inode from the writeback list it is on, except
 * potentially to move it from b_dirty_time to b_dirty due to timestamp
 * expiration.  The caller is otherwise responsible for writeback list handling.
 *
 * The caller is also responsible for setting the I_SYNC flag beforehand and
 * calling inode_sync_complete() to clear it afterwards.
 */
static int
__writeback_single_inode(struct inode *inode, struct writeback_control *wbc)
{
	struct address_space *mapping = inode->i_mapping;
	unsigned dirty;
	int ret;

	WARN_ON(!(inode_state_read_once(inode) & I_SYNC));


	ret = do_writepages(mapping, wbc);

	/*
	 * Make sure to wait on the data before writing out the metadata.
	 * This is important for filesystems that modify metadata on data
	 * I/O completion. We don't do it for sync(2) writeback because it has a
	 * separate, external IO completion path and ->sync_fs for guaranteeing
	 * inode metadata is written back correctly.
	 */
	if (wbc->sync_mode == WB_SYNC_ALL && !wbc->for_sync) {
		int err = filemap_fdatawait(mapping);
		if (ret == 0)
			ret = err;
	}

	/*
	 * For data integrity writeback, or when the dirty interval expired,
	 * ask the file system to propagata lazy timestamp updates into real
	 * dirty state.
	 */
	if ((inode_state_read_once(inode) & I_DIRTY_TIME) &&
	    (wbc->sync_mode == WB_SYNC_ALL ||
	     time_after(jiffies, inode->dirtied_time_when +
			dirtytime_expire_interval * HZ)))
		sync_lazytime(inode);

	/*
	 * Get and clear the dirty flags from i_state.  This needs to be done
	 * after calling writepages because some filesystems may redirty the
	 * inode during writepages due to delalloc.  It also needs to be done
	 * after handling timestamp expiration, as that may dirty the inode too.
	 */
	spin_lock(&inode->i_lock);
	dirty = inode_state_read(inode) & I_DIRTY;
	inode_state_clear(inode, dirty);

	/*
	 * Paired with smp_mb() in __mark_inode_dirty().  This allows
	 * __mark_inode_dirty() to test i_state without grabbing i_lock -
	 * either they see the I_DIRTY bits cleared or we see the dirtied
	 * inode.
	 *
	 * I_DIRTY_PAGES is always cleared together above even if @mapping
	 * still has dirty pages.  The flag is reinstated after smp_mb() if
	 * necessary.  This guarantees that either __mark_inode_dirty()
	 * sees clear I_DIRTY_PAGES or we see PAGECACHE_TAG_DIRTY.
	 */
	smp_mb();

	if (mapping_tagged(mapping, PAGECACHE_TAG_DIRTY))
		inode_state_set(inode, I_DIRTY_PAGES);
	else if (unlikely(inode_state_read(inode) & I_PINNING_NETFS_WB)) {
		if (!(inode_state_read(inode) & I_DIRTY_PAGES)) {
			inode_state_clear(inode, I_PINNING_NETFS_WB);
			wbc->unpinned_netfs_wb = true;
			dirty |= I_PINNING_NETFS_WB; /* Cause write_inode */
		}
	}

	spin_unlock(&inode->i_lock);

	/* Don't write the inode if only I_DIRTY_PAGES was set */
	if (dirty & ~I_DIRTY_PAGES) {
		int err = write_inode(inode, wbc);
		if (ret == 0)
			ret = err;
	}
	wbc->unpinned_netfs_wb = false;
	return ret;
}

/*
 * Write out an inode's dirty data and metadata on-demand, i.e. separately from
 * the regular batched writeback done by the flusher threads in
 * writeback_sb_inodes().  @wbc controls various aspects of the write, such as
 * whether it is a data-integrity sync (%WB_SYNC_ALL) or not (%WB_SYNC_NONE).
 *
 * To prevent the inode from going away, either the caller must have a reference
 * to the inode, or the inode must have I_WILL_FREE or I_FREEING set.
 */
static int writeback_single_inode(struct inode *inode,
				  struct writeback_control *wbc)
{
	struct bdi_writeback *wb;
	int ret = 0;

	spin_lock(&inode->i_lock);
	if (!icount_read(inode))
		WARN_ON(!(inode_state_read(inode) & (I_WILL_FREE | I_FREEING)));
	else
		WARN_ON(inode_state_read(inode) & I_WILL_FREE);

	if (inode_state_read(inode) & I_SYNC) {
		/*
		 * Writeback is already running on the inode.  For WB_SYNC_NONE,
		 * that's enough and we can just return.  For WB_SYNC_ALL, we
		 * must wait for the existing writeback to complete, then do
		 * writeback again if there's anything left.
		 */
		if (wbc->sync_mode != WB_SYNC_ALL)
			goto out;
		inode_wait_for_writeback(inode);
	}
	WARN_ON(inode_state_read(inode) & I_SYNC);
	/*
	 * If the inode is already fully clean, then there's nothing to do.
	 *
	 * For data-integrity syncs we also need to check whether any pages are
	 * still under writeback, e.g. due to prior WB_SYNC_NONE writeback.  If
	 * there are any such pages, we'll need to wait for them.
	 */
	if (!(inode_state_read(inode) & I_DIRTY_ALL) &&
	    (wbc->sync_mode != WB_SYNC_ALL ||
	     !mapping_tagged(inode->i_mapping, PAGECACHE_TAG_WRITEBACK)))
		goto out;
	inode_state_set(inode, I_SYNC);
	wbc_attach_and_unlock_inode(wbc, inode);

	ret = __writeback_single_inode(inode, wbc);

	wb = inode_to_wb_and_lock_list(inode);
	spin_lock(&inode->i_lock);
	/*
	 * If the inode is freeing, its i_io_list shoudn't be updated
	 * as it can be finally deleted at this moment.
	 */
	if (!(inode_state_read(inode) & I_FREEING)) {
		/*
		 * If the inode is now fully clean, then it can be safely
		 * removed from its writeback list (if any). Otherwise the
		 * flusher threads are responsible for the writeback lists.
		 */
		if (!(inode_state_read(inode) & I_DIRTY_ALL))
			inode_cgwb_move_to_attached(inode, wb);
		else if (!(inode_state_read(inode) & I_SYNC_QUEUED)) {
			if ((inode_state_read(inode) & I_DIRTY))
				redirty_tail_locked(inode, wb);
			else if (inode_state_read(inode) & I_DIRTY_TIME) {
				inode->dirtied_when = jiffies;
				inode_io_list_move_locked(inode,
							  wb,
							  &wb->b_dirty_time);
			}
		}
	}

	spin_unlock(&wb->list_lock);
	inode_sync_complete(inode);
out:
	spin_unlock(&inode->i_lock);
	return ret;
}

static long writeback_chunk_size(struct super_block *sb,
		struct bdi_writeback *wb, struct wb_writeback_work *work)
{
	long pages;

	/*
	 * WB_SYNC_ALL mode does livelock avoidance by syncing dirty
	 * inodes/pages in one big loop. Setting wbc.nr_to_write=LONG_MAX
	 * here avoids calling into writeback_inodes_wb() more than once.
	 *
	 * The intended call sequence for WB_SYNC_ALL writeback is:
	 *
	 *      wb_writeback()
	 *          writeback_sb_inodes()       <== called only once
	 *              write_cache_pages()     <== called once for each inode
	 *                   (quickly) tag currently dirty pages
	 *                   (maybe slowly) sync all tagged pages
	 */
	if (work->sync_mode == WB_SYNC_ALL || work->tagged_writepages)
		return LONG_MAX;

	pages = min(wb->avg_write_bandwidth / 2,
		    global_wb_domain.dirty_limit / DIRTY_SCOPE);
	pages = min(pages, work->nr_pages);
	return round_down(pages + sb->s_min_writeback_pages,
			sb->s_min_writeback_pages);
}

/*
 * Write a portion of b_io inodes which belong to @sb.
 *
 * Return the number of pages and/or inodes written.
 *
 * NOTE! This is called with wb->list_lock held, and will
 * unlock and relock that for each inode it ends up doing
 * IO for.
 */
static long writeback_sb_inodes(struct super_block *sb,
				struct bdi_writeback *wb,
				struct wb_writeback_work *work)
{
	struct writeback_control wbc = {
		.sync_mode		= work->sync_mode,
		.tagged_writepages	= work->tagged_writepages,
		.for_kupdate		= work->for_kupdate,
		.for_background		= work->for_background,
		.for_sync		= work->for_sync,
		.range_cyclic		= work->range_cyclic,
		.range_start		= 0,
		.range_end		= LLONG_MAX,
	};
	unsigned long start_time = jiffies;
	unsigned long timeout = sysctl_hung_task_timeout_secs;
	long write_chunk;
	long total_wrote = 0;  /* count both pages and inodes */
	unsigned long dirtied_before = jiffies;

	if (work->for_kupdate)
		dirtied_before = jiffies -
			msecs_to_jiffies(dirty_expire_interval * 10);

	while (!list_empty(&wb->b_io)) {
		struct inode *inode = wb_inode(wb->b_io.prev);
		struct bdi_writeback *tmp_wb;
		long wrote;

		if (inode->i_sb != sb) {
			if (work->sb) {
				/*
				 * We only want to write back data for this
				 * superblock, move all inodes not belonging
				 * to it back onto the dirty list.
				 */
				redirty_tail(inode, wb);
				continue;
			}

			/*
			 * The inode belongs to a different superblock.
			 * Bounce back to the caller to unpin this and
			 * pin the next superblock.
			 */
			break;
		}

		/*
		 * Don't bother with new inodes or inodes being freed, first
		 * kind does not need periodic writeout yet, and for the latter
		 * kind writeout is handled by the freer.
		 */
		spin_lock(&inode->i_lock);
		if (inode_state_read(inode) & (I_NEW | I_FREEING | I_WILL_FREE)) {
			redirty_tail_locked(inode, wb);
			spin_unlock(&inode->i_lock);
			continue;
		}
		if ((inode_state_read(inode) & I_SYNC) && wbc.sync_mode != WB_SYNC_ALL) {
			/*
			 * If this inode is locked for writeback and we are not
			 * doing writeback-for-data-integrity, move it to
			 * b_more_io so that writeback can proceed with the
			 * other inodes on s_io.
			 *
			 * We'll have another go at writing back this inode
			 * when we completed a full scan of b_io.
			 */
			requeue_io(inode, wb);
			spin_unlock(&inode->i_lock);
			continue;
		}
		spin_unlock(&wb->list_lock);

		/*
		 * We already requeued the inode if it had I_SYNC set and we
		 * are doing WB_SYNC_NONE writeback. So this catches only the
		 * WB_SYNC_ALL case.
		 */
		if (inode_state_read(inode) & I_SYNC) {
			/* Wait for I_SYNC. This function drops i_lock... */
			inode_sleep_on_writeback(inode);
			/* Inode may be gone, start again */
			spin_lock(&wb->list_lock);
			continue;
		}
		inode_state_set(inode, I_SYNC);
		wbc_attach_and_unlock_inode(&wbc, inode);

		write_chunk = writeback_chunk_size(inode->i_sb, wb, work);
		wbc.nr_to_write = write_chunk;
		wbc.pages_skipped = 0;

		/*
		 * We use I_SYNC to pin the inode in memory. While it is set
		 * evict_inode() will wait so the inode cannot be freed.
		 */
		__writeback_single_inode(inode, &wbc);

		/* Report progress to inform the hung task detector of the progress. */
		if (work->done && work->done->progress_stamp && timeout &&
		   (jiffies - work->done->progress_stamp) > HZ * timeout / 2)
			wake_up_all(work->done->waitq);

		work->nr_pages -= write_chunk - wbc.nr_to_write;
		wrote = write_chunk - wbc.nr_to_write - wbc.pages_skipped;
		wrote = wrote < 0 ? 0 : wrote;
		total_wrote += wrote;

		if (need_resched()) {
			/*
			 * We're trying to balance between building up a nice
			 * long list of IOs to improve our merge rate, and
			 * getting those IOs out quickly for anyone throttling
			 * in balance_dirty_pages().  cond_resched() doesn't
			 * unplug, so get our IOs out the door before we
			 * give up the CPU.
			 */
			blk_flush_plug(current->plug, false);
			cond_resched();
		}

		/*
		 * Requeue @inode if still dirty.  Be careful as @inode may
		 * have been switched to another wb in the meantime.
		 */
		tmp_wb = inode_to_wb_and_lock_list(inode);
		spin_lock(&inode->i_lock);
		if (!(inode_state_read(inode) & I_DIRTY_ALL))
			total_wrote++;
		requeue_inode(inode, tmp_wb, &wbc, dirtied_before);
		inode_sync_complete(inode);
		spin_unlock(&inode->i_lock);

		if (unlikely(tmp_wb != wb)) {
			spin_unlock(&tmp_wb->list_lock);
			spin_lock(&wb->list_lock);
		}

		/*
		 * bail out to wb_writeback() often enough to check
		 * background threshold and other termination conditions.
		 */
		if (total_wrote) {
			if (time_is_before_jiffies(start_time + HZ / 10UL))
				break;
			if (work->nr_pages <= 0)
				break;
		}
	}
	return total_wrote;
}

static long __writeback_inodes_wb(struct bdi_writeback *wb,
				  struct wb_writeback_work *work)
{
	unsigned long start_time = jiffies;
	long wrote = 0;

	while (!list_empty(&wb->b_io)) {
		struct inode *inode = wb_inode(wb->b_io.prev);
		struct super_block *sb = inode->i_sb;

		if (!super_trylock_shared(sb)) {
			/*
			 * super_trylock_shared() may fail consistently due to
			 * s_umount being grabbed by someone else. Don't use
			 * requeue_io() to avoid busy retrying the inode/sb.
			 */
			redirty_tail(inode, wb);
			continue;
		}
		wrote += writeback_sb_inodes(sb, wb, work);
		up_read(&sb->s_umount);

		/* refer to the same tests at the end of writeback_sb_inodes */
		if (wrote) {
			if (time_is_before_jiffies(start_time + HZ / 10UL))
				break;
			if (work->nr_pages <= 0)
				break;
		}
	}
	/* Leave any unwritten inodes on b_io */
	return wrote;
}

static long writeback_inodes_wb(struct bdi_writeback *wb, long nr_pages,
				enum wb_reason reason)
{
	struct wb_writeback_work work = {
		.nr_pages	= nr_pages,
		.sync_mode	= WB_SYNC_NONE,
		.range_cyclic	= 1,
		.reason		= reason,
	};
	struct blk_plug plug;

	blk_start_plug(&plug);
	spin_lock(&wb->list_lock);
	if (list_empty(&wb->b_io))
		queue_io(wb, &work, jiffies);
	__writeback_inodes_wb(wb, &work);
	spin_unlock(&wb->list_lock);
	blk_finish_plug(&plug);

	return nr_pages - work.nr_pages;
}

/*
 * Explicit flushing or periodic writeback of "old" data.
 *
 * Define "old": the first time one of an inode's pages is dirtied, we mark the
 * dirtying-time in the inode's address_space.  So this periodic writeback code
 * just walks the superblock inode list, writing back any inodes which are
 * older than a specific point in time.
 *
 * Try to run once per dirty_writeback_interval.  But if a writeback event
 * takes longer than a dirty_writeback_interval interval, then leave a
 * one-second gap.
 *
 * dirtied_before takes precedence over nr_to_write.  So we'll only write back
 * all dirty pages if they are all attached to "old" mappings.
 */
static long wb_writeback(struct bdi_writeback *wb,
			 struct wb_writeback_work *work)
{
	long nr_pages = work->nr_pages;
	unsigned long dirtied_before = jiffies;
	struct inode *inode;
	long progress;
	struct blk_plug plug;
	bool queued = false;

	blk_start_plug(&plug);
	for (;;) {
		/*
		 * Stop writeback when nr_pages has been consumed
		 */
		if (work->nr_pages <= 0)
			break;

		/*
		 * Background writeout and kupdate-style writeback may
		 * run forever. Stop them if there is other work to do
		 * so that e.g. sync can proceed. They'll be restarted
		 * after the other works are all done.
		 */
		if ((work->for_background || work->for_kupdate) &&
		    !list_empty(&wb->work_list))
			break;

		/*
		 * For background writeout, stop when we are below the
		 * background dirty threshold
		 */
		if (work->for_background && !wb_over_bg_thresh(wb))
			break;


		spin_lock(&wb->list_lock);

		if (list_empty(&wb->b_io)) {
			/*
			 * Kupdate and background works are special and we want
			 * to include all inodes that need writing. Livelock
			 * avoidance is handled by these works yielding to any
			 * other work so we are safe.
			 */
			if (work->for_kupdate) {
				dirtied_before = jiffies -
					msecs_to_jiffies(dirty_expire_interval *
							 10);
			} else if (work->for_background)
				dirtied_before = jiffies;

			queue_io(wb, work, dirtied_before);
			queued = true;
		}
		if (work->sb)
			progress = writeback_sb_inodes(work->sb, wb, work);
		else
			progress = __writeback_inodes_wb(wb, work);

		/*
		 * Did we write something? Try for more
		 *
		 * Dirty inodes are moved to b_io for writeback in batches.
		 * The completion of the current batch does not necessarily
		 * mean the overall work is done. So we keep looping as long
		 * as made some progress on cleaning pages or inodes.
		 */
		if (progress || !queued) {
			spin_unlock(&wb->list_lock);
			continue;
		}

		/*
		 * No more inodes for IO, bail
		 */
		if (list_empty(&wb->b_more_io)) {
			spin_unlock(&wb->list_lock);
			break;
		}

		/*
		 * Nothing written. Wait for some inode to
		 * become available for writeback. Otherwise
		 * we'll just busyloop.
		 */
		inode = wb_inode(wb->b_more_io.prev);
		spin_lock(&inode->i_lock);
		spin_unlock(&wb->list_lock);
		/* This function drops i_lock... */
		inode_sleep_on_writeback(inode);
	}
	blk_finish_plug(&plug);

	return nr_pages - work->nr_pages;
}

/*
 * Return the next wb_writeback_work struct that hasn't been processed yet.
 */
static struct wb_writeback_work *get_next_work_item(struct bdi_writeback *wb)
{
	struct wb_writeback_work *work = NULL;

	spin_lock_irq(&wb->work_lock);
	if (!list_empty(&wb->work_list)) {
		work = list_entry(wb->work_list.next,
				  struct wb_writeback_work, list);
		list_del_init(&work->list);
	}
	spin_unlock_irq(&wb->work_lock);
	return work;
}

static long wb_check_background_flush(struct bdi_writeback *wb)
{
	if (wb_over_bg_thresh(wb)) {

		struct wb_writeback_work work = {
			.nr_pages	= LONG_MAX,
			.sync_mode	= WB_SYNC_NONE,
			.for_background	= 1,
			.range_cyclic	= 1,
			.reason		= WB_REASON_BACKGROUND,
		};

		return wb_writeback(wb, &work);
	}

	return 0;
}

static long wb_check_old_data_flush(struct bdi_writeback *wb)
{
	unsigned long expired;
	long nr_pages;

	/*
	 * When set to zero, disable periodic writeback
	 */
	if (!dirty_writeback_interval)
		return 0;

	expired = wb->last_old_flush +
			msecs_to_jiffies(dirty_writeback_interval * 10);
	if (time_before(jiffies, expired))
		return 0;

	wb->last_old_flush = jiffies;
	nr_pages = get_nr_dirty_pages();

	if (nr_pages) {
		struct wb_writeback_work work = {
			.nr_pages	= nr_pages,
			.sync_mode	= WB_SYNC_NONE,
			.for_kupdate	= 1,
			.range_cyclic	= 1,
			.reason		= WB_REASON_PERIODIC,
		};

		return wb_writeback(wb, &work);
	}

	return 0;
}

static long wb_check_start_all(struct bdi_writeback *wb)
{
	long nr_pages;

	if (!test_bit(WB_start_all, &wb->state))
		return 0;

	nr_pages = get_nr_dirty_pages();
	if (nr_pages) {
		struct wb_writeback_work work = {
			.nr_pages	= nr_pages,
			.sync_mode	= WB_SYNC_NONE,
			.range_cyclic	= 1,
			.reason		= wb->start_all_reason,
		};

		nr_pages = wb_writeback(wb, &work);
	}

	clear_bit(WB_start_all, &wb->state);
	return nr_pages;
}


/*
 * Retrieve work items and do the writeback they describe
 */
static long wb_do_writeback(struct bdi_writeback *wb)
{
	struct wb_writeback_work *work;
	long wrote = 0;

	set_bit(WB_writeback_running, &wb->state);
	while ((work = get_next_work_item(wb)) != NULL) {
		wrote += wb_writeback(wb, work);
		finish_writeback_work(work);
	}

	/*
	 * Check for a flush-everything request
	 */
	wrote += wb_check_start_all(wb);

	/*
	 * Check for periodic writeback, kupdated() style
	 */
	wrote += wb_check_old_data_flush(wb);
	wrote += wb_check_background_flush(wb);
	clear_bit(WB_writeback_running, &wb->state);

	return wrote;
}

/*
 * Handle writeback of dirty data for the device backed by this bdi. Also
 * reschedules periodically and does kupdated style flushing.
 */
void wb_workfn(struct work_struct *work)
{
	struct bdi_writeback *wb = container_of(to_delayed_work(work),
						struct bdi_writeback, dwork);
	long pages_written;

	set_worker_desc("flush-%s", bdi_dev_name(wb->bdi));

	if (likely(!current_is_workqueue_rescuer() ||
		   !test_bit(WB_registered, &wb->state))) {
		/*
		 * The normal path.  Keep writing back @wb until its
		 * work_list is empty.  Note that this path is also taken
		 * if @wb is shutting down even when we're running off the
		 * rescuer as work_list needs to be drained.
		 */
		do {
			pages_written = wb_do_writeback(wb);
		} while (!list_empty(&wb->work_list));
	} else {
		/*
		 * bdi_wq can't get enough workers and we're running off
		 * the emergency worker.  Don't hog it.  Hopefully, 1024 is
		 * enough for efficient IO.
		 */
		pages_written = writeback_inodes_wb(wb, 1024,
						    WB_REASON_FORKER_THREAD);
	}

	if (!list_empty(&wb->work_list))
		wb_wakeup(wb);
	else if (wb_has_dirty_io(wb) && dirty_writeback_interval)
		wb_wakeup_delayed(wb);
}

/*
 * Start writeback of all dirty pages on this bdi.
 */
static void __wakeup_flusher_threads_bdi(struct backing_dev_info *bdi,
					 enum wb_reason reason)
{
	struct bdi_writeback *wb;

	if (!bdi_has_dirty_io(bdi))
		return;

	list_for_each_entry_rcu(wb, &bdi->wb_list, bdi_node)
		wb_start_writeback(wb, reason);
}

void wakeup_flusher_threads_bdi(struct backing_dev_info *bdi,
				enum wb_reason reason)
{
	rcu_read_lock();
	__wakeup_flusher_threads_bdi(bdi, reason);
	rcu_read_unlock();
}

/*
 * Wakeup the flusher threads to start writeback of all currently dirty pages
 */
void wakeup_flusher_threads(enum wb_reason reason)
{
	struct backing_dev_info *bdi;

	/*
	 * If we are expecting writeback progress we must submit plugged IO.
	 */
	blk_flush_plug(current->plug, true);

	rcu_read_lock();
	list_for_each_entry_rcu(bdi, &bdi_list, bdi_list)
		__wakeup_flusher_threads_bdi(bdi, reason);
	rcu_read_unlock();
}

/*
 * Wake up bdi's periodically to make sure dirtytime inodes gets
 * written back periodically.  We deliberately do *not* check the
 * b_dirtytime list in wb_has_dirty_io(), since this would cause the
 * kernel to be constantly waking up once there are any dirtytime
 * inodes on the system.  So instead we define a separate delayed work
 * function which gets called much more rarely.  (By default, only
 * once every 12 hours.)
 *
 * If there is any other write activity going on in the file system,
 * this function won't be necessary.  But if the only thing that has
 * happened on the file system is a dirtytime inode caused by an atime
 * update, we need this infrastructure below to make sure that inode
 * eventually gets pushed out to disk.
 */
static void wakeup_dirtytime_writeback(struct work_struct *w);
static DECLARE_DELAYED_WORK(dirtytime_work, wakeup_dirtytime_writeback);

static void wakeup_dirtytime_writeback(struct work_struct *w)
{
	struct backing_dev_info *bdi;

	rcu_read_lock();
	list_for_each_entry_rcu(bdi, &bdi_list, bdi_list) {
		struct bdi_writeback *wb;

		list_for_each_entry_rcu(wb, &bdi->wb_list, bdi_node)
			if (!list_empty(&wb->b_dirty_time))
				wb_wakeup(wb);
	}
	rcu_read_unlock();
	if (dirtytime_expire_interval)
		schedule_delayed_work(&dirtytime_work,
				      round_jiffies_relative(dirtytime_expire_interval * HZ));
}

static int dirtytime_interval_handler(const struct ctl_table *table, int write,
			       void *buffer, size_t *lenp, loff_t *ppos)
{
	int ret;

	ret = proc_dointvec_minmax(table, write, buffer, lenp, ppos);
	if (ret == 0 && write) {
		if (dirtytime_expire_interval)
			mod_delayed_work(system_percpu_wq, &dirtytime_work, 0);
		else
			cancel_delayed_work_sync(&dirtytime_work);
	}
	return ret;
}

static const struct ctl_table vm_fs_writeback_table[] = {
	{
		.procname	= "dirtytime_expire_seconds",
		.data		= &dirtytime_expire_interval,
		.maxlen		= sizeof(dirtytime_expire_interval),
		.mode		= 0644,
		.proc_handler	= dirtytime_interval_handler,
		.extra1		= SYSCTL_ZERO,
	},
};

static int __init start_dirtytime_writeback(void)
{
	if (dirtytime_expire_interval)
		schedule_delayed_work(&dirtytime_work,
				      round_jiffies_relative(dirtytime_expire_interval * HZ));
	register_sysctl_init("vm", vm_fs_writeback_table);
	return 0;
}
__initcall(start_dirtytime_writeback);

/**
 * __mark_inode_dirty -	internal function to mark an inode dirty
 *
 * @inode: inode to mark
 * @flags: what kind of dirty, e.g. I_DIRTY_SYNC.  This can be a combination of
 *	   multiple I_DIRTY_* flags, except that I_DIRTY_TIME can't be combined
 *	   with I_DIRTY_PAGES.
 *
 * Mark an inode as dirty.  We notify the filesystem, then update the inode's
 * dirty flags.  Then, if needed we add the inode to the appropriate dirty list.
 *
 * Most callers should use mark_inode_dirty() or mark_inode_dirty_sync()
 * instead of calling this directly.
 *
 * CAREFUL!  We only add the inode to the dirty list if it is hashed or if it
 * refers to a blockdev.  Unhashed inodes will never be added to the dirty list
 * even if they are later hashed, as they will have been marked dirty already.
 *
 * In short, ensure you hash any inodes _before_ you start marking them dirty.
 *
 * Note that for blockdevs, inode->dirtied_when represents the dirtying time of
 * the block-special inode (/dev/hda1) itself.  And the ->dirtied_when field of
 * the kernel-internal blockdev inode represents the dirtying time of the
 * blockdev's pages.  This is why for I_DIRTY_PAGES we always use
 * page->mapping->host, so the page-dirtying time is recorded in the internal
 * blockdev inode.
 */
void __mark_inode_dirty(struct inode *inode, int flags)
{
	struct super_block *sb = inode->i_sb;
	int dirtytime = 0;
	struct bdi_writeback *wb = NULL;


	if (flags & I_DIRTY_INODE) {
		bool was_dirty_time = false;

		/*
		 * Inode timestamp update will piggback on this dirtying.
		 * We tell ->dirty_inode callback that timestamps need to
		 * be updated by setting I_DIRTY_TIME in flags.
		 */
		if (inode_state_read_once(inode) & I_DIRTY_TIME) {
			spin_lock(&inode->i_lock);
			if (inode_state_read(inode) & I_DIRTY_TIME) {
				inode_state_clear(inode, I_DIRTY_TIME);
				flags |= I_DIRTY_TIME;
				was_dirty_time = true;
			}
			spin_unlock(&inode->i_lock);
		}

		/*
		 * Notify the filesystem about the inode being dirtied, so that
		 * (if needed) it can update on-disk fields and journal the
		 * inode.  This is only needed when the inode itself is being
		 * dirtied now.  I.e. it's only needed for I_DIRTY_INODE, not
		 * for just I_DIRTY_PAGES or I_DIRTY_TIME.
		 */
		if (sb->s_op->dirty_inode) {
			sb->s_op->dirty_inode(inode,
				flags & (I_DIRTY_INODE | I_DIRTY_TIME));
		} else if (was_dirty_time && inode->i_op->sync_lazytime) {
			inode->i_op->sync_lazytime(inode);
		}

		/* I_DIRTY_INODE supersedes I_DIRTY_TIME. */
		flags &= ~I_DIRTY_TIME;
	} else {
		/*
		 * Else it's either I_DIRTY_PAGES, I_DIRTY_TIME, or nothing.
		 * (We don't support setting both I_DIRTY_PAGES and I_DIRTY_TIME
		 * in one call to __mark_inode_dirty().)
		 */
		dirtytime = flags & I_DIRTY_TIME;
		WARN_ON_ONCE(dirtytime && flags != I_DIRTY_TIME);
	}

	/*
	 * Paired with smp_mb() in __writeback_single_inode() for the
	 * following lockless i_state test.  See there for details.
	 */
	smp_mb();

	if ((inode_state_read_once(inode) & flags) == flags)
		return;

	spin_lock(&inode->i_lock);
	if ((inode_state_read(inode) & flags) != flags) {
		const int was_dirty = inode_state_read(inode) & I_DIRTY;

		inode_attach_wb(inode, NULL);

		inode_state_set(inode, flags);

		/*
		 * Grab inode's wb early because it requires dropping i_lock and we
		 * need to make sure following checks happen atomically with dirty
		 * list handling so that we don't move inodes under flush worker's
		 * hands.
		 */
		if (!was_dirty) {
			wb = locked_inode_to_wb_and_lock_list(inode);
			spin_lock(&inode->i_lock);
		}

		/*
		 * If the inode is queued for writeback by flush worker, just
		 * update its dirty state. Once the flush worker is done with
		 * the inode it will place it on the appropriate superblock
		 * list, based upon its state.
		 */
		if (inode_state_read(inode) & I_SYNC_QUEUED)
			goto out_unlock;

		/*
		 * Only add valid (hashed) inodes to the superblock's
		 * dirty list.  Add blockdev inodes as well.
		 */
		if (!S_ISBLK(inode->i_mode)) {
			if (inode_unhashed(inode))
				goto out_unlock;
		}
		if (inode_state_read(inode) & I_FREEING)
			goto out_unlock;

		/*
		 * If the inode was already on b_dirty/b_io/b_more_io, don't
		 * reposition it (that would break b_dirty time-ordering).
		 */
		if (!was_dirty) {
			struct list_head *dirty_list;
			bool wakeup_bdi = false;

			inode->dirtied_when = jiffies;
			if (dirtytime)
				inode->dirtied_time_when = jiffies;

			if (inode_state_read(inode) & I_DIRTY)
				dirty_list = &wb->b_dirty;
			else
				dirty_list = &wb->b_dirty_time;

			wakeup_bdi = inode_io_list_move_locked(inode, wb,
							       dirty_list);

			/*
			 * If this is the first dirty inode for this bdi,
			 * we have to wake-up the corresponding bdi thread
			 * to make sure background write-back happens
			 * later.
			 */
			if (wakeup_bdi &&
			    (wb->bdi->capabilities & BDI_CAP_WRITEBACK))
				wb_wakeup_delayed(wb);

			spin_unlock(&wb->list_lock);
			spin_unlock(&inode->i_lock);

			return;
		}
	}
out_unlock:
	if (wb)
		spin_unlock(&wb->list_lock);
	spin_unlock(&inode->i_lock);
}
EXPORT_SYMBOL(__mark_inode_dirty);

/*
 * The @s_sync_lock is used to serialise concurrent sync operations
 * to avoid lock contention problems with concurrent wait_sb_inodes() calls.
 * Concurrent callers will block on the s_sync_lock rather than doing contending
 * walks. The queueing maintains sync(2) required behaviour as all the IO that
 * has been issued up to the time this function is enter is guaranteed to be
 * completed by the time we have gained the lock and waited for all IO that is
 * in progress regardless of the order callers are granted the lock.
 */
static void wait_sb_inodes(struct super_block *sb)
{
	LIST_HEAD(sync_list);

	/*
	 * We need to be protected against the filesystem going from
	 * r/o to r/w or vice versa.
	 */
	WARN_ON(!rwsem_is_locked(&sb->s_umount));

	mutex_lock(&sb->s_sync_lock);

	/*
	 * Splice the writeback list onto a temporary list to avoid waiting on
	 * inodes that have started writeback after this point.
	 *
	 * Use rcu_read_lock() to keep the inodes around until we have a
	 * reference. s_inode_wblist_lock protects sb->s_inodes_wb as well as
	 * the local list because inodes can be dropped from either by writeback
	 * completion.
	 */
	rcu_read_lock();
	spin_lock_irq(&sb->s_inode_wblist_lock);
	list_splice_init(&sb->s_inodes_wb, &sync_list);

	/*
	 * Data integrity sync. Must wait for all pages under writeback, because
	 * there may have been pages dirtied before our sync call, but which had
	 * writeout started before we write it out.  In which case, the inode
	 * may not be on the dirty list, but we still have to wait for that
	 * writeout.
	 */
	while (!list_empty(&sync_list)) {
		struct inode *inode = list_first_entry(&sync_list, struct inode,
						       i_wb_list);
		struct address_space *mapping = inode->i_mapping;

		/*
		 * Move each inode back to the wb list before we drop the lock
		 * to preserve consistency between i_wb_list and the mapping
		 * writeback tag. Writeback completion is responsible to remove
		 * the inode from either list once the writeback tag is cleared.
		 */
		list_move_tail(&inode->i_wb_list, &sb->s_inodes_wb);

		/*
		 * The mapping can appear untagged while still on-list since we
		 * do not have the mapping lock. Skip it here, wb completion
		 * will remove it.
		 */
		if (!mapping_tagged(mapping, PAGECACHE_TAG_WRITEBACK))
			continue;

		spin_unlock_irq(&sb->s_inode_wblist_lock);

		spin_lock(&inode->i_lock);
		if (inode_state_read(inode) & (I_FREEING | I_WILL_FREE | I_NEW)) {
			spin_unlock(&inode->i_lock);

			spin_lock_irq(&sb->s_inode_wblist_lock);
			continue;
		}
		__iget(inode);
		spin_unlock(&inode->i_lock);
		rcu_read_unlock();

		/*
		 * We keep the error status of individual mapping so that
		 * applications can catch the writeback error using fsync(2).
		 * See filemap_fdatawait_keep_errors() for details.
		 */
		filemap_fdatawait_keep_errors(mapping);

		cond_resched();

		iput(inode);

		rcu_read_lock();
		spin_lock_irq(&sb->s_inode_wblist_lock);
	}
	spin_unlock_irq(&sb->s_inode_wblist_lock);
	rcu_read_unlock();
	mutex_unlock(&sb->s_sync_lock);
}

static void __writeback_inodes_sb_nr(struct super_block *sb, unsigned long nr,
				     enum wb_reason reason, bool skip_if_busy)
{
	struct backing_dev_info *bdi = sb->s_bdi;
	DEFINE_WB_COMPLETION(done, bdi);
	struct wb_writeback_work work = {
		.sb			= sb,
		.sync_mode		= WB_SYNC_NONE,
		.tagged_writepages	= 1,
		.done			= &done,
		.nr_pages		= nr,
		.reason			= reason,
	};

	if (!bdi_has_dirty_io(bdi) || bdi == &noop_backing_dev_info)
		return;
	WARN_ON(!rwsem_is_locked(&sb->s_umount));

	bdi_split_work_to_wbs(sb->s_bdi, &work, skip_if_busy);
	wb_wait_for_completion(&done);
}

/**
 * writeback_inodes_sb_nr -	writeback dirty inodes from given super_block
 * @sb: the superblock
 * @nr: the number of pages to write
 * @reason: reason why some writeback work initiated
 *
 * Start writeback on some inodes on this super_block. No guarantees are made
 * on how many (if any) will be written, and this function does not wait
 * for IO completion of submitted IO.
 */
void writeback_inodes_sb_nr(struct super_block *sb,
			    unsigned long nr,
			    enum wb_reason reason)
{
	__writeback_inodes_sb_nr(sb, nr, reason, false);
}
EXPORT_SYMBOL(writeback_inodes_sb_nr);

/**
 * writeback_inodes_sb	-	writeback dirty inodes from given super_block
 * @sb: the superblock
 * @reason: reason why some writeback work was initiated
 *
 * Start writeback on some inodes on this super_block. No guarantees are made
 * on how many (if any) will be written, and this function does not wait
 * for IO completion of submitted IO.
 */
void writeback_inodes_sb(struct super_block *sb, enum wb_reason reason)
{
	writeback_inodes_sb_nr(sb, get_nr_dirty_pages(), reason);
}
EXPORT_SYMBOL(writeback_inodes_sb);

/**
 * try_to_writeback_inodes_sb - try to start writeback if none underway
 * @sb: the superblock
 * @reason: reason why some writeback work was initiated
 *
 * Invoke __writeback_inodes_sb_nr if no writeback is currently underway.
 */
void try_to_writeback_inodes_sb(struct super_block *sb, enum wb_reason reason)
{
	if (!down_read_trylock(&sb->s_umount))
		return;

	__writeback_inodes_sb_nr(sb, get_nr_dirty_pages(), reason, true);
	up_read(&sb->s_umount);
}
EXPORT_SYMBOL(try_to_writeback_inodes_sb);

/**
 * sync_inodes_sb	-	sync sb inode pages
 * @sb: the superblock
 *
 * This function writes and waits on any dirty inode belonging to this
 * super_block.
 */
void sync_inodes_sb(struct super_block *sb)
{
	struct backing_dev_info *bdi = sb->s_bdi;
	DEFINE_WB_COMPLETION(done, bdi);
	struct wb_writeback_work work = {
		.sb		= sb,
		.sync_mode	= WB_SYNC_ALL,
		.nr_pages	= LONG_MAX,
		.range_cyclic	= 0,
		.done		= &done,
		.reason		= WB_REASON_SYNC,
		.for_sync	= 1,
	};

	/*
	 * Can't skip on !bdi_has_dirty() because we should wait for !dirty
	 * inodes under writeback and I_DIRTY_TIME inodes ignored by
	 * bdi_has_dirty() need to be written out too.
	 */
	if (bdi == &noop_backing_dev_info)
		return;

	/*
	 * If the superblock has SB_I_NO_DATA_INTEGRITY set, there's no need to
	 * wait for the writeout to complete, as the filesystem cannot guarantee
	 * data persistence on sync. Just kick off writeback and return.
	 */
	if (sb->s_iflags & SB_I_NO_DATA_INTEGRITY) {
		wakeup_flusher_threads_bdi(bdi, WB_REASON_SYNC);
		return;
	}

	WARN_ON(!rwsem_is_locked(&sb->s_umount));

	/* protect against inode wb switch, see inode_switch_wbs_work_fn() */
	bdi_down_write_wb_switch_rwsem(bdi);
	bdi_split_work_to_wbs(bdi, &work, false);
	wb_wait_for_completion(&done);
	bdi_up_write_wb_switch_rwsem(bdi);

	wait_sb_inodes(sb);
}
EXPORT_SYMBOL(sync_inodes_sb);

/**
 * write_inode_now	-	write an inode to disk
 * @inode: inode to write to disk
 * @sync: whether the write should be synchronous or not
 *
 * This function commits an inode to disk immediately if it is dirty. This is
 * primarily needed by knfsd.
 *
 * The caller must either have a ref on the inode or must have set I_WILL_FREE.
 */
int write_inode_now(struct inode *inode, int sync)
{
	struct writeback_control wbc = {
		.nr_to_write = LONG_MAX,
		.sync_mode = sync ? WB_SYNC_ALL : WB_SYNC_NONE,
		.range_start = 0,
		.range_end = LLONG_MAX,
	};

	if (!mapping_can_writeback(inode->i_mapping))
		wbc.nr_to_write = 0;

	might_sleep();
	return writeback_single_inode(inode, &wbc);
}
EXPORT_SYMBOL(write_inode_now);

/**
 * sync_inode_metadata - write an inode to disk
 * @inode: the inode to sync
 * @wait: wait for I/O to complete.
 *
 * Write an inode to disk and adjust its dirty state after completion.
 *
 * Note: only writes the actual inode, no associated data or other metadata.
 */
int sync_inode_metadata(struct inode *inode, int wait)
{
	struct writeback_control wbc = {
		.sync_mode = wait ? WB_SYNC_ALL : WB_SYNC_NONE,
		.nr_to_write = 0, /* metadata-only */
	};

	return writeback_single_inode(inode, &wbc);
}
EXPORT_SYMBOL(sync_inode_metadata);
