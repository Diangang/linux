// SPDX-License-Identifier: GPL-2.0
/*
 * Resizable virtual memory filesystem for Linux.
 *
 * Copyright (C) 2000 Linus Torvalds.
 *		 2000 Transmeta Corp.
 *		 2000-2001 Christoph Rohland
 *		 2000-2001 SAP AG
 *		 2002 Red Hat Inc.
 * Copyright (C) 2002-2011 Hugh Dickins.
 * Copyright (C) 2011 Google Inc.
 * Copyright (C) 2002-2005 VERITAS Software Corporation.
 * Copyright (C) 2004 Andi Kleen, SuSE Labs
 *
 * Extended attribute support for tmpfs:
 * Copyright (c) 2004, Luke Kenneth Casson Leighton <lkcl@lkcl.net>
 * Copyright (c) 2004 Red Hat, Inc., James Morris <jmorris@redhat.com>
 *
 * tiny-shmem:
 * Copyright (c) 2004, 2008 Matt Mackall <mpm@selenic.com>
 */

#include <linux/fs.h>
#include <linux/init.h>
#include <linux/vfs.h>
#include <linux/mount.h>
#include <linux/ramfs.h>
#include <linux/pagemap.h>
#include <linux/file.h>
#include <linux/fileattr.h>
#include <linux/filelock.h>
#include <linux/mm.h>
#include <linux/random.h>
#include <linux/sched/signal.h>
#include <linux/export.h>
#include <linux/shmem_fs.h>
#include <linux/swap.h>
#include <linux/uio.h>
#include <linux/hugetlb.h>
#include <linux/fs_parser.h>
#include <linux/swapfile.h>
#include <linux/iversion.h>
#include <linux/unicode.h>
#include "swap.h"

static struct vfsmount *shm_mnt __ro_after_init;

#ifdef CONFIG_SHMEM
/*
 * This virtual memory filesystem is heavily based on the ramfs. It
 * extends ramfs by the ability to use swap and honor resource limits
 * which makes it a completely usable filesystem.
 */

#include <linux/xattr.h>
#include <linux/exportfs.h>
#include <linux/mman.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/backing-dev.h>
#include <linux/writeback.h>
#include <linux/folio_batch.h>
#include <linux/percpu_counter.h>
#include <linux/falloc.h>
#include <linux/splice.h>
#include <linux/security.h>
#include <linux/leafops.h>
#include <linux/mempolicy.h>
#include <linux/namei.h>
#include <linux/ctype.h>
#include <linux/migrate.h>
#include <linux/highmem.h>
#include <linux/seq_file.h>
#include <linux/magic.h>
#include <linux/syscalls.h>
#include <linux/fcntl.h>
#include <uapi/linux/memfd.h>
#include <linux/rmap.h>
#include <linux/uuid.h>
#include <linux/quotaops.h>
#include <linux/rcupdate_wait.h>

#include <linux/uaccess.h>

#include "internal.h"

#define VM_ACCT(size)    (PAGE_ALIGN(size) >> PAGE_SHIFT)

/* Pretend that each entry is of this size in directory's i_size */
#define BOGO_DIRENT_SIZE 20

/* Pretend that one inode + its dentry occupy this much memory */
#define BOGO_INODE_SIZE 1024

/* Symlink up to this size is kmalloc'ed instead of using a swappable page */
#define SHORT_SYMLINK_LEN 128

/*
 * shmem_fallocate communicates with shmem_fault or shmem_writeout via
 * inode->i_private (with i_rwsem making sure that it has only one user at
 * a time): we would prefer not to enlarge the shmem inode just for that.
 */
struct shmem_falloc {
	wait_queue_head_t *waitq; /* faults into hole wait for punch to end */
	pgoff_t start;		/* start of range currently being fallocated */
	pgoff_t next;		/* the next page offset to be fallocated */
	pgoff_t nr_falloced;	/* how many new pages have been fallocated */
	pgoff_t nr_unswapped;	/* how often writeout refused to swap out */
};

struct shmem_options {
	unsigned long long blocks;
	unsigned long long inodes;
	struct mempolicy *mpol;
	kuid_t uid;
	kgid_t gid;
	umode_t mode;
	bool full_inums;
	int huge;
	int seen;
	bool noswap;
	unsigned short quota_types;
	struct shmem_quota_limits qlimits;
#define SHMEM_SEEN_BLOCKS 1
#define SHMEM_SEEN_INODES 2
#define SHMEM_SEEN_HUGE 4
#define SHMEM_SEEN_INUMS 8
#define SHMEM_SEEN_QUOTA 16
};



static int shmem_swapin_folio(struct inode *inode, pgoff_t index,
			struct folio **foliop, enum sgp_type sgp, gfp_t gfp,
			struct vm_area_struct *vma, vm_fault_t *fault_type);

static inline struct shmem_sb_info *SHMEM_SB(struct super_block *sb)
{
	return sb->s_fs_info;
}

/*
 * shmem_file_setup pre-accounts the whole fixed size of a VM object,
 * for shared memory and for shared anonymous (/dev/zero) mappings
 * (unless MAP_NORESERVE and sysctl_overcommit_memory <= 1),
 * consistent with the pre-accounting of private mappings ...
 */
static inline int shmem_acct_size(unsigned long flags, loff_t size)
{
	return (flags & SHMEM_F_NORESERVE) ?
		0 : security_vm_enough_memory_mm(current->mm, VM_ACCT(size));
}

static inline void shmem_unacct_size(unsigned long flags, loff_t size)
{
	if (!(flags & SHMEM_F_NORESERVE))
		vm_unacct_memory(VM_ACCT(size));
}

static inline int shmem_reacct_size(unsigned long flags,
		loff_t oldsize, loff_t newsize)
{
	if (!(flags & SHMEM_F_NORESERVE)) {
		if (VM_ACCT(newsize) > VM_ACCT(oldsize))
			return security_vm_enough_memory_mm(current->mm,
					VM_ACCT(newsize) - VM_ACCT(oldsize));
		else if (VM_ACCT(newsize) < VM_ACCT(oldsize))
			vm_unacct_memory(VM_ACCT(oldsize) - VM_ACCT(newsize));
	}
	return 0;
}

/*
 * ... whereas tmpfs objects are accounted incrementally as
 * pages are allocated, in order to allow large sparse files.
 * shmem_get_folio reports shmem_acct_blocks failure as -ENOSPC not -ENOMEM,
 * so that a failure on a sparse tmpfs mapping will give SIGBUS not OOM.
 */
static inline int shmem_acct_blocks(unsigned long flags, long pages)
{
	if (!(flags & SHMEM_F_NORESERVE))
		return 0;

	return security_vm_enough_memory_mm(current->mm,
			pages * VM_ACCT(PAGE_SIZE));
}

static inline void shmem_unacct_blocks(unsigned long flags, long pages)
{
	if (flags & SHMEM_F_NORESERVE)
		vm_unacct_memory(pages * VM_ACCT(PAGE_SIZE));
}

int shmem_inode_acct_blocks(struct inode *inode, long pages)
{
	struct shmem_inode_info *info = SHMEM_I(inode);
	struct shmem_sb_info *sbinfo = SHMEM_SB(inode->i_sb);
	int err = -ENOSPC;

	if (shmem_acct_blocks(info->flags, pages))
		return err;

	might_sleep();	/* when quotas */
	if (sbinfo->max_blocks) {
		if (!percpu_counter_limited_add(&sbinfo->used_blocks,
						sbinfo->max_blocks, pages))
			goto unacct;

		err = dquot_alloc_block_nodirty(inode, pages);
		if (err) {
			percpu_counter_sub(&sbinfo->used_blocks, pages);
			goto unacct;
		}
	} else {
		err = dquot_alloc_block_nodirty(inode, pages);
		if (err)
			goto unacct;
	}

	return 0;

unacct:
	shmem_unacct_blocks(info->flags, pages);
	return err;
}

static void shmem_inode_unacct_blocks(struct inode *inode, long pages)
{
	struct shmem_inode_info *info = SHMEM_I(inode);
	struct shmem_sb_info *sbinfo = SHMEM_SB(inode->i_sb);

	might_sleep();	/* when quotas */
	dquot_free_block_nodirty(inode, pages);

	if (sbinfo->max_blocks)
		percpu_counter_sub(&sbinfo->used_blocks, pages);
	shmem_unacct_blocks(info->flags, pages);
}

static const struct super_operations shmem_ops;
static const struct address_space_operations shmem_aops;
static const struct file_operations shmem_file_operations;
static const struct inode_operations shmem_inode_operations;
static const struct inode_operations shmem_dir_inode_operations;
static const struct inode_operations shmem_special_inode_operations;
static const struct vm_operations_struct shmem_vm_ops;
static const struct vm_operations_struct shmem_anon_vm_ops;
static struct file_system_type shmem_fs_type;

bool shmem_mapping(const struct address_space *mapping)
{
	return mapping->a_ops == &shmem_aops;
}
EXPORT_SYMBOL_GPL(shmem_mapping);

bool vma_is_anon_shmem(const struct vm_area_struct *vma)
{
	return vma->vm_ops == &shmem_anon_vm_ops;
}

bool vma_is_shmem(const struct vm_area_struct *vma)
{
	return vma_is_anon_shmem(vma) || vma->vm_ops == &shmem_vm_ops;
}

static LIST_HEAD(shmem_swaplist);
static DEFINE_SPINLOCK(shmem_swaplist_lock);


/*
 * shmem_reserve_inode() performs bookkeeping to reserve a shmem inode, and
 * produces a novel ino for the newly allocated inode.
 *
 * It may also be called when making a hard link to permit the space needed by
 * each dentry. However, in that case, no new inode number is needed since that
 * internally draws from another pool of inode numbers (currently global
 * get_next_ino()). This case is indicated by passing NULL as inop.
 */
#define SHMEM_INO_BATCH 1024
static int shmem_reserve_inode(struct super_block *sb, ino_t *inop)
{
	struct shmem_sb_info *sbinfo = SHMEM_SB(sb);
	ino_t ino;

	if (!(sb->s_flags & SB_KERNMOUNT)) {
		raw_spin_lock(&sbinfo->stat_lock);
		if (sbinfo->max_inodes) {
			if (sbinfo->free_ispace < BOGO_INODE_SIZE) {
				raw_spin_unlock(&sbinfo->stat_lock);
				return -ENOSPC;
			}
			sbinfo->free_ispace -= BOGO_INODE_SIZE;
		}
		if (inop) {
			ino = sbinfo->next_ino++;
			if (unlikely(is_zero_ino(ino)))
				ino = sbinfo->next_ino++;
			if (unlikely(!sbinfo->full_inums &&
				     ino > UINT_MAX)) {
				/*
				 * Emulate get_next_ino uint wraparound for
				 * compatibility
				 */
				if (IS_ENABLED(CONFIG_64BIT))
					pr_warn("%s: inode number overflow on device %d, consider using inode64 mount option\n",
						__func__, MINOR(sb->s_dev));
				sbinfo->next_ino = 1;
				ino = sbinfo->next_ino++;
			}
			*inop = ino;
		}
		raw_spin_unlock(&sbinfo->stat_lock);
	} else if (inop) {
		/*
		 * __shmem_file_setup, one of our callers, is lock-free: it
		 * doesn't hold stat_lock in shmem_reserve_inode since
		 * max_inodes is always 0, and is called from potentially
		 * unknown contexts. As such, use a per-cpu batched allocator
		 * which doesn't require the per-sb stat_lock unless we are at
		 * the batch boundary.
		 *
		 * We don't need to worry about inode{32,64} since SB_KERNMOUNT
		 * shmem mounts are not exposed to userspace, so we don't need
		 * to worry about things like glibc compatibility.
		 */
		ino_t *next_ino;

		next_ino = per_cpu_ptr(sbinfo->ino_batch, get_cpu());
		ino = *next_ino;
		if (unlikely(ino % SHMEM_INO_BATCH == 0)) {
			raw_spin_lock(&sbinfo->stat_lock);
			ino = sbinfo->next_ino;
			sbinfo->next_ino += SHMEM_INO_BATCH;
			raw_spin_unlock(&sbinfo->stat_lock);
			if (unlikely(is_zero_ino(ino)))
				ino++;
		}
		*inop = ino;
		*next_ino = ++ino;
		put_cpu();
	}

	return 0;
}

static void shmem_free_inode(struct super_block *sb, size_t freed_ispace)
{
	struct shmem_sb_info *sbinfo = SHMEM_SB(sb);
	if (sbinfo->max_inodes) {
		raw_spin_lock(&sbinfo->stat_lock);
		sbinfo->free_ispace += BOGO_INODE_SIZE + freed_ispace;
		raw_spin_unlock(&sbinfo->stat_lock);
	}
}

/**
 * shmem_recalc_inode - recalculate the block usage of an inode
 * @inode: inode to recalc
 * @alloced: the change in number of pages allocated to inode
 * @swapped: the change in number of pages swapped from inode
 *
 * We have to calculate the free blocks since the mm can drop
 * undirtied hole pages behind our back.
 *
 * But normally   info->alloced == inode->i_mapping->nrpages + info->swapped
 * So mm freed is info->alloced - (inode->i_mapping->nrpages + info->swapped)
 *
 * Return: true if swapped was incremented from 0, for shmem_writeout().
 */
bool shmem_recalc_inode(struct inode *inode, long alloced, long swapped)
{
	struct shmem_inode_info *info = SHMEM_I(inode);
	bool first_swapped = false;
	long freed;

	spin_lock(&info->lock);
	info->alloced += alloced;
	info->swapped += swapped;
	freed = info->alloced - info->swapped -
		READ_ONCE(inode->i_mapping->nrpages);
	/*
	 * Special case: whereas normally shmem_recalc_inode() is called
	 * after i_mapping->nrpages has already been adjusted (up or down),
	 * shmem_writeout() has to raise swapped before nrpages is lowered -
	 * to stop a racing shmem_recalc_inode() from thinking that a page has
	 * been freed.  Compensate here, to avoid the need for a followup call.
	 */
	if (swapped > 0) {
		if (info->swapped == swapped)
			first_swapped = true;
		freed += swapped;
	}
	if (freed > 0)
		info->alloced -= freed;
	spin_unlock(&info->lock);

	/* The quota case may block */
	if (freed > 0)
		shmem_inode_unacct_blocks(inode, freed);
	return first_swapped;
}

bool shmem_charge(struct inode *inode, long pages)
{
	struct address_space *mapping = inode->i_mapping;

	if (shmem_inode_acct_blocks(inode, pages))
		return false;

	/* nrpages adjustment first, then shmem_recalc_inode() when balanced */
	xa_lock_irq(&mapping->i_pages);
	mapping->nrpages += pages;
	xa_unlock_irq(&mapping->i_pages);

	shmem_recalc_inode(inode, pages, 0);
	return true;
}

void shmem_uncharge(struct inode *inode, long pages)
{
	/* pages argument is currently unused: keep it to help debugging */
	/* nrpages adjustment done by __filemap_remove_folio() or caller */

	shmem_recalc_inode(inode, 0, 0);
}

/*
 * Replace item expected in xarray by a new item, while holding xa_lock.
 */
static int shmem_replace_entry(struct address_space *mapping,
			pgoff_t index, void *expected, void *replacement)
{
	XA_STATE(xas, &mapping->i_pages, index);
	void *item;

	VM_BUG_ON(!expected);
	VM_BUG_ON(!replacement);
	item = xas_load(&xas);
	if (item != expected)
		return -ENOENT;
	xas_store(&xas, replacement);
	return 0;
}

/*
 * Sometimes, before we decide whether to proceed or to fail, we must check
 * that an entry was not already brought back or split by a racing thread.
 *
 * Checking folio is not enough: by the time a swapcache folio is locked, it
 * might be reused, and again be swapcache, using the same swap as before.
 * Returns the swap entry's order if it still presents, else returns -1.
 */
static int shmem_confirm_swap(struct address_space *mapping, pgoff_t index,
			      swp_entry_t swap)
{
	XA_STATE(xas, &mapping->i_pages, index);
	int ret = -1;
	void *entry;

	rcu_read_lock();
	do {
		entry = xas_load(&xas);
		if (entry == swp_to_radix_entry(swap))
			ret = xas_get_order(&xas);
	} while (xas_retry(&xas, entry));
	rcu_read_unlock();
	return ret;
}

/*
 * Definitions for "huge tmpfs": tmpfs mounted with the huge= option
 *
 * SHMEM_HUGE_NEVER:
 *	disables huge pages for the mount;
 * SHMEM_HUGE_ALWAYS:
 *	enables huge pages for the mount;
 * SHMEM_HUGE_WITHIN_SIZE:
 *	only allocate huge pages if the page will be fully within i_size,
 *	also respect madvise() hints;
 * SHMEM_HUGE_ADVISE:
 *	only allocate huge pages if requested with madvise();
 */

#define SHMEM_HUGE_NEVER	0
#define SHMEM_HUGE_ALWAYS	1
#define SHMEM_HUGE_WITHIN_SIZE	2
#define SHMEM_HUGE_ADVISE	3

/*
 * Special values.
 * Only can be set via /sys/kernel/mm/transparent_hugepage/shmem_enabled:
 *
 * SHMEM_HUGE_DENY:
 *	disables huge on shm_mnt and all mounts, for emergency use;
 * SHMEM_HUGE_FORCE:
 *	enables huge on shm_mnt and all mounts, w/o needing option, for testing;
 *
 */
#define SHMEM_HUGE_DENY		(-1)
#define SHMEM_HUGE_FORCE	(-2)


#define shmem_huge SHMEM_HUGE_DENY

static unsigned long shmem_unused_huge_shrink(struct shmem_sb_info *sbinfo,
		struct shrink_control *sc, unsigned long nr_to_free)
{
	return 0;
}

static unsigned int shmem_huge_global_enabled(struct inode *inode, pgoff_t index,
					      loff_t write_end, bool shmem_huge_force,
					      struct vm_area_struct *vma,
					      vm_flags_t vm_flags)
{
	return 0;
}

static void shmem_update_stats(struct folio *folio, int nr_pages)
{
	if (folio_test_pmd_mappable(folio))
		lruvec_stat_mod_folio(folio, NR_SHMEM_THPS, nr_pages);
	lruvec_stat_mod_folio(folio, NR_FILE_PAGES, nr_pages);
	lruvec_stat_mod_folio(folio, NR_SHMEM, nr_pages);
}

/*
 * Somewhat like filemap_add_folio, but error if expected item has gone.
 */
int shmem_add_to_page_cache(struct folio *folio,
			    struct address_space *mapping,
			    pgoff_t index, void *expected, gfp_t gfp)
{
	XA_STATE_ORDER(xas, &mapping->i_pages, index, folio_order(folio));
	unsigned long nr = folio_nr_pages(folio);
	swp_entry_t iter, swap;
	void *entry;

	VM_BUG_ON_FOLIO(index != round_down(index, nr), folio);
	VM_BUG_ON_FOLIO(!folio_test_locked(folio), folio);
	VM_BUG_ON_FOLIO(!folio_test_swapbacked(folio), folio);

	folio_ref_add(folio, nr);
	folio->mapping = mapping;
	folio->index = index;

	gfp &= GFP_RECLAIM_MASK;
	folio_throttle_swaprate(folio, gfp);
	swap = radix_to_swp_entry(expected);

	do {
		iter = swap;
		xas_lock_irq(&xas);
		xas_for_each_conflict(&xas, entry) {
			/*
			 * The range must either be empty, or filled with
			 * expected swap entries. Shmem swap entries are never
			 * partially freed without split of both entry and
			 * folio, so there shouldn't be any holes.
			 */
			if (!expected || entry != swp_to_radix_entry(iter)) {
				xas_set_err(&xas, -EEXIST);
				goto unlock;
			}
			iter.val += 1 << xas_get_order(&xas);
		}
		if (expected && iter.val - nr != swap.val) {
			xas_set_err(&xas, -EEXIST);
			goto unlock;
		}
		xas_store(&xas, folio);
		if (xas_error(&xas))
			goto unlock;
		shmem_update_stats(folio, nr);
		mapping->nrpages += nr;
unlock:
		xas_unlock_irq(&xas);
	} while (xas_nomem(&xas, gfp));

	if (xas_error(&xas)) {
		folio->mapping = NULL;
		folio_ref_sub(folio, nr);
		return xas_error(&xas);
	}

	return 0;
}

/*
 * Somewhat like filemap_remove_folio, but substitutes swap for @folio.
 */
static void shmem_delete_from_page_cache(struct folio *folio, void *radswap)
{
	struct address_space *mapping = folio->mapping;
	long nr = folio_nr_pages(folio);
	int error;

	xa_lock_irq(&mapping->i_pages);
	error = shmem_replace_entry(mapping, folio->index, folio, radswap);
	folio->mapping = NULL;
	mapping->nrpages -= nr;
	shmem_update_stats(folio, -nr);
	xa_unlock_irq(&mapping->i_pages);
	folio_put_refs(folio, nr);
	BUG_ON(error);
}

/*
 * Remove swap entry from page cache, free the swap and its page cache. Returns
 * the number of pages being freed. 0 means entry not found in XArray (0 pages
 * being freed).
 */
static long shmem_free_swap(struct address_space *mapping,
			    pgoff_t index, pgoff_t end, void *radswap)
{
	XA_STATE(xas, &mapping->i_pages, index);
	unsigned int nr_pages = 0;
	pgoff_t base;
	void *entry;

	xas_lock_irq(&xas);
	entry = xas_load(&xas);
	if (entry == radswap) {
		nr_pages = 1 << xas_get_order(&xas);
		base = round_down(xas.xa_index, nr_pages);
		if (base < index || base + nr_pages - 1 > end)
			nr_pages = 0;
		else
			xas_store(&xas, NULL);
	}
	xas_unlock_irq(&xas);

	if (nr_pages)
		swap_put_entries_direct(radix_to_swp_entry(radswap), nr_pages);

	return nr_pages;
}

/*
 * Determine (in bytes) how many of the shmem object's pages mapped by the
 * given offsets are swapped out.
 *
 * This is safe to call without i_rwsem or the i_pages lock thanks to RCU,
 * as long as the inode doesn't go away and racy results are not a problem.
 */
unsigned long shmem_partial_swap_usage(struct address_space *mapping,
						pgoff_t start, pgoff_t end)
{
	XA_STATE(xas, &mapping->i_pages, start);
	struct folio *folio;
	unsigned long swapped = 0;
	unsigned long max = end - 1;

	rcu_read_lock();
	xas_for_each(&xas, folio, max) {
		if (xas_retry(&xas, folio))
			continue;
		if (xa_is_value(folio))
			swapped += 1 << xas_get_order(&xas);
		if (xas.xa_index == max)
			break;
		if (need_resched()) {
			xas_pause(&xas);
			cond_resched_rcu();
		}
	}
	rcu_read_unlock();

	return swapped << PAGE_SHIFT;
}

/*
 * Determine (in bytes) how many of the shmem object's pages mapped by the
 * given vma is swapped out.
 *
 * This is safe to call without i_rwsem or the i_pages lock thanks to RCU,
 * as long as the inode doesn't go away and racy results are not a problem.
 */
unsigned long shmem_swap_usage(struct vm_area_struct *vma)
{
	struct inode *inode = file_inode(vma->vm_file);
	struct shmem_inode_info *info = SHMEM_I(inode);
	struct address_space *mapping = inode->i_mapping;
	unsigned long swapped;

	/* Be careful as we don't hold info->lock */
	swapped = READ_ONCE(info->swapped);

	/*
	 * The easier cases are when the shmem object has nothing in swap, or
	 * the vma maps it whole. Then we can simply use the stats that we
	 * already track.
	 */
	if (!swapped)
		return 0;

	if (!vma->vm_pgoff && vma->vm_end - vma->vm_start >= inode->i_size)
		return swapped << PAGE_SHIFT;

	/* Here comes the more involved part */
	return shmem_partial_swap_usage(mapping, vma->vm_pgoff,
					vma->vm_pgoff + vma_pages(vma));
}

/*
 * SysV IPC SHM_UNLOCK restore Unevictable pages to their evictable lists.
 */
void shmem_unlock_mapping(struct address_space *mapping)
{
	struct folio_batch fbatch;
	pgoff_t index = 0;

	folio_batch_init(&fbatch);
	/*
	 * Minor point, but we might as well stop if someone else SHM_LOCKs it.
	 */
	while (!mapping_unevictable(mapping) &&
	       filemap_get_folios(mapping, &index, ~0UL, &fbatch)) {
		check_move_unevictable_folios(&fbatch);
		folio_batch_release(&fbatch);
		cond_resched();
	}
}

static struct folio *shmem_get_partial_folio(struct inode *inode, pgoff_t index)
{
	struct folio *folio;

	/*
	 * At first avoid shmem_get_folio(,,,SGP_READ): that fails
	 * beyond i_size, and reports fallocated folios as holes.
	 */
	folio = filemap_get_entry(inode->i_mapping, index);
	if (!folio)
		return folio;
	if (!xa_is_value(folio)) {
		folio_lock(folio);
		if (folio->mapping == inode->i_mapping)
			return folio;
		/* The folio has been swapped out */
		folio_unlock(folio);
		folio_put(folio);
	}
	/*
	 * But read a folio back from swap if any of it is within i_size
	 * (although in some cases this is just a waste of time).
	 */
	folio = NULL;
	shmem_get_folio(inode, index, 0, &folio, SGP_READ);
	return folio;
}

/*
 * Remove range of pages and swap entries from page cache, and free them.
 * If !unfalloc, truncate or punch hole; if unfalloc, undo failed fallocate.
 */
static void shmem_undo_range(struct inode *inode, loff_t lstart, uoff_t lend,
								 bool unfalloc)
{
	struct address_space *mapping = inode->i_mapping;
	struct shmem_inode_info *info = SHMEM_I(inode);
	pgoff_t start = (lstart + PAGE_SIZE - 1) >> PAGE_SHIFT;
	pgoff_t end = (lend + 1) >> PAGE_SHIFT;
	struct folio_batch fbatch;
	pgoff_t indices[FOLIO_BATCH_SIZE];
	struct folio *folio;
	bool same_folio;
	long nr_swaps_freed = 0;
	pgoff_t index;
	int i;

	if (lend == -1)
		end = -1;	/* unsigned, so actually very big */

	if (info->fallocend > start && info->fallocend <= end && !unfalloc)
		info->fallocend = start;

	folio_batch_init(&fbatch);
	index = start;
	while (index < end && find_lock_entries(mapping, &index, end - 1,
			&fbatch, indices)) {
		for (i = 0; i < folio_batch_count(&fbatch); i++) {
			folio = fbatch.folios[i];

			if (xa_is_value(folio)) {
				if (unfalloc)
					continue;
				nr_swaps_freed += shmem_free_swap(mapping, indices[i],
								  end - 1, folio);
				continue;
			}

			if (!unfalloc || !folio_test_uptodate(folio))
				truncate_inode_folio(mapping, folio);
			folio_unlock(folio);
		}
		folio_batch_remove_exceptionals(&fbatch);
		folio_batch_release(&fbatch);
		cond_resched();
	}

	/*
	 * When undoing a failed fallocate, we want none of the partial folio
	 * zeroing and splitting below, but shall want to truncate the whole
	 * folio when !uptodate indicates that it was added by this fallocate,
	 * even when [lstart, lend] covers only a part of the folio.
	 */
	if (unfalloc)
		goto whole_folios;

	same_folio = (lstart >> PAGE_SHIFT) == (lend >> PAGE_SHIFT);
	folio = shmem_get_partial_folio(inode, lstart >> PAGE_SHIFT);
	if (folio) {
		same_folio = lend < folio_next_pos(folio);
		folio_mark_dirty(folio);
		if (!truncate_inode_partial_folio(folio, lstart, lend)) {
			start = folio_next_index(folio);
			if (same_folio)
				end = folio->index;
		}
		folio_unlock(folio);
		folio_put(folio);
		folio = NULL;
	}

	if (!same_folio)
		folio = shmem_get_partial_folio(inode, lend >> PAGE_SHIFT);
	if (folio) {
		folio_mark_dirty(folio);
		if (!truncate_inode_partial_folio(folio, lstart, lend))
			end = folio->index;
		folio_unlock(folio);
		folio_put(folio);
	}

whole_folios:

	index = start;
	while (index < end) {
		cond_resched();

		if (!find_get_entries(mapping, &index, end - 1, &fbatch,
				indices)) {
			/* If all gone or hole-punch or unfalloc, we're done */
			if (index == start || end != -1)
				break;
			/* But if truncating, restart to make sure all gone */
			index = start;
			continue;
		}
		for (i = 0; i < folio_batch_count(&fbatch); i++) {
			folio = fbatch.folios[i];

			if (xa_is_value(folio)) {
				int order;
				long swaps_freed;

				if (unfalloc)
					continue;
				swaps_freed = shmem_free_swap(mapping, indices[i],
							      end - 1, folio);
				if (!swaps_freed) {
					pgoff_t base = indices[i];

					order = shmem_confirm_swap(mapping, indices[i],
								   radix_to_swp_entry(folio));
					/*
					 * If found a large swap entry cross the end or start
					 * border, skip it as the truncate_inode_partial_folio
					 * above should have at least zerod its content once.
					 */
					if (order > 0) {
						base = round_down(base, 1 << order);
						if (base < start || base + (1 << order) > end)
							continue;
					}
					/* Swap was replaced by page or extended, retry */
					index = base;
					break;
				}
				nr_swaps_freed += swaps_freed;
				continue;
			}

			folio_lock(folio);

			if (!unfalloc || !folio_test_uptodate(folio)) {
				if (folio_mapping(folio) != mapping) {
					/* Page was replaced by swap: retry */
					folio_unlock(folio);
					index = indices[i];
					break;
				}
				VM_BUG_ON_FOLIO(folio_test_writeback(folio),
						folio);

				if (!folio_test_large(folio)) {
					truncate_inode_folio(mapping, folio);
				} else if (truncate_inode_partial_folio(folio, lstart, lend)) {
					/*
					 * If we split a page, reset the loop so
					 * that we pick up the new sub pages.
					 * Otherwise the THP was entirely
					 * dropped or the target range was
					 * zeroed, so just continue the loop as
					 * is.
					 */
					if (!folio_test_large(folio)) {
						folio_unlock(folio);
						index = start;
						break;
					}
				}
			}
			folio_unlock(folio);
		}
		folio_batch_remove_exceptionals(&fbatch);
		folio_batch_release(&fbatch);
	}

	shmem_recalc_inode(inode, 0, -nr_swaps_freed);
}

void shmem_truncate_range(struct inode *inode, loff_t lstart, uoff_t lend)
{
	shmem_undo_range(inode, lstart, lend, false);
	inode_set_mtime_to_ts(inode, inode_set_ctime_current(inode));
	inode_inc_iversion(inode);
}
EXPORT_SYMBOL_GPL(shmem_truncate_range);

static int shmem_getattr(struct mnt_idmap *idmap,
			 const struct path *path, struct kstat *stat,
			 u32 request_mask, unsigned int query_flags)
{
	struct inode *inode = path->dentry->d_inode;
	struct shmem_inode_info *info = SHMEM_I(inode);

	if (info->alloced - info->swapped != inode->i_mapping->nrpages)
		shmem_recalc_inode(inode, 0, 0);

	if (info->fsflags & FS_APPEND_FL)
		stat->attributes |= STATX_ATTR_APPEND;
	if (info->fsflags & FS_IMMUTABLE_FL)
		stat->attributes |= STATX_ATTR_IMMUTABLE;
	if (info->fsflags & FS_NODUMP_FL)
		stat->attributes |= STATX_ATTR_NODUMP;
	stat->attributes_mask |= (STATX_ATTR_APPEND |
			STATX_ATTR_IMMUTABLE |
			STATX_ATTR_NODUMP);
	generic_fillattr(idmap, request_mask, inode, stat);

	if (shmem_huge_global_enabled(inode, 0, 0, false, NULL, 0))
		stat->blksize = HPAGE_PMD_SIZE;

	if (request_mask & STATX_BTIME) {
		stat->result_mask |= STATX_BTIME;
		stat->btime.tv_sec = info->i_crtime.tv_sec;
		stat->btime.tv_nsec = info->i_crtime.tv_nsec;
	}

	return 0;
}

static int shmem_setattr(struct mnt_idmap *idmap,
			 struct dentry *dentry, struct iattr *attr)
{
	struct inode *inode = d_inode(dentry);
	struct shmem_inode_info *info = SHMEM_I(inode);
	int error;
	bool update_mtime = false;
	bool update_ctime = true;

	error = setattr_prepare(idmap, dentry, attr);
	if (error)
		return error;

	if ((info->seals & F_SEAL_EXEC) && (attr->ia_valid & ATTR_MODE)) {
		if ((inode->i_mode ^ attr->ia_mode) & 0111) {
			return -EPERM;
		}
	}

	if (S_ISREG(inode->i_mode) && (attr->ia_valid & ATTR_SIZE)) {
		loff_t oldsize = inode->i_size;
		loff_t newsize = attr->ia_size;

		/* protected by i_rwsem */
		if ((newsize < oldsize && (info->seals & F_SEAL_SHRINK)) ||
		    (newsize > oldsize && (info->seals & F_SEAL_GROW)))
			return -EPERM;

		if (newsize != oldsize) {
			if (info->flags & SHMEM_F_MAPPING_FROZEN)
				return -EPERM;
			error = shmem_reacct_size(SHMEM_I(inode)->flags,
					oldsize, newsize);
			if (error)
				return error;
			i_size_write(inode, newsize);
			update_mtime = true;
		} else {
			update_ctime = false;
		}
		if (newsize <= oldsize) {
			loff_t holebegin = round_up(newsize, PAGE_SIZE);
			if (oldsize > holebegin)
				unmap_mapping_range(inode->i_mapping,
							holebegin, 0, 1);
			if (info->alloced)
				shmem_truncate_range(inode,
							newsize, (loff_t)-1);
			/* unmap again to remove racily COWed private pages */
			if (oldsize > holebegin)
				unmap_mapping_range(inode->i_mapping,
							holebegin, 0, 1);
		}
	}

	if (is_quota_modification(idmap, inode, attr)) {
		error = dquot_initialize(inode);
		if (error)
			return error;
	}

	/* Transfer quota accounting */
	if (i_uid_needs_update(idmap, attr, inode) ||
	    i_gid_needs_update(idmap, attr, inode)) {
		error = dquot_transfer(idmap, inode, attr);
		if (error)
			return error;
	}

	setattr_copy(idmap, inode, attr);
	if (update_ctime) {
		inode_set_ctime_current(inode);
		if (update_mtime)
			inode_set_mtime_to_ts(inode, inode_get_ctime(inode));
		inode_inc_iversion(inode);
	}
	return 0;
}

static void shmem_evict_inode(struct inode *inode)
{
	struct shmem_inode_info *info = SHMEM_I(inode);
	struct shmem_sb_info *sbinfo = SHMEM_SB(inode->i_sb);
	size_t freed = 0;

	if (shmem_mapping(inode->i_mapping)) {
		shmem_unacct_size(info->flags, inode->i_size);
		inode->i_size = 0;
		mapping_set_exiting(inode->i_mapping);
		shmem_truncate_range(inode, 0, (loff_t)-1);
		if (!list_empty(&info->shrinklist)) {
			spin_lock(&sbinfo->shrinklist_lock);
			if (!list_empty(&info->shrinklist)) {
				list_del_init(&info->shrinklist);
				sbinfo->shrinklist_len--;
			}
			spin_unlock(&sbinfo->shrinklist_lock);
		}
		while (!list_empty(&info->swaplist)) {
			/* Wait while shmem_unuse() is scanning this inode... */
			wait_var_event(&info->stop_eviction,
				       !atomic_read(&info->stop_eviction));
			spin_lock(&shmem_swaplist_lock);
			/* ...but beware of the race if we peeked too early */
			if (!atomic_read(&info->stop_eviction))
				list_del_init(&info->swaplist);
			spin_unlock(&shmem_swaplist_lock);
		}
	}

	if (info->xattrs) {
		simple_xattrs_free(info->xattrs, sbinfo->max_inodes ? &freed : NULL);
		kfree(info->xattrs);
	}
	shmem_free_inode(inode->i_sb, freed);
	WARN_ON(inode->i_blocks);
	clear_inode(inode);
}

static unsigned int shmem_find_swap_entries(struct address_space *mapping,
				pgoff_t start, struct folio_batch *fbatch,
				pgoff_t *indices, unsigned int type)
{
	XA_STATE(xas, &mapping->i_pages, start);
	struct folio *folio;
	swp_entry_t entry;

	rcu_read_lock();
	xas_for_each(&xas, folio, ULONG_MAX) {
		if (xas_retry(&xas, folio))
			continue;

		if (!xa_is_value(folio))
			continue;

		entry = radix_to_swp_entry(folio);
		/*
		 * swapin error entries can be found in the mapping. But they're
		 * deliberately ignored here as we've done everything we can do.
		 */
		if (swp_type(entry) != type)
			continue;

		indices[folio_batch_count(fbatch)] = xas.xa_index;
		if (!folio_batch_add(fbatch, folio))
			break;

		if (need_resched()) {
			xas_pause(&xas);
			cond_resched_rcu();
		}
	}
	rcu_read_unlock();

	return folio_batch_count(fbatch);
}

/*
 * Move the swapped pages for an inode to page cache. Returns the count
 * of pages swapped in, or the error in case of failure.
 */
static int shmem_unuse_swap_entries(struct inode *inode,
		struct folio_batch *fbatch, pgoff_t *indices)
{
	int i = 0;
	int ret = 0;
	int error = 0;
	struct address_space *mapping = inode->i_mapping;

	for (i = 0; i < folio_batch_count(fbatch); i++) {
		struct folio *folio = fbatch->folios[i];

		error = shmem_swapin_folio(inode, indices[i], &folio, SGP_CACHE,
					mapping_gfp_mask(mapping), NULL, NULL);
		if (error == 0) {
			folio_unlock(folio);
			folio_put(folio);
			ret++;
		}
		if (error == -ENOMEM)
			break;
		error = 0;
	}
	return error ? error : ret;
}

/*
 * If swap found in inode, free it and move page from swapcache to filecache.
 */
static int shmem_unuse_inode(struct inode *inode, unsigned int type)
{
	struct address_space *mapping = inode->i_mapping;
	pgoff_t start = 0;
	struct folio_batch fbatch;
	pgoff_t indices[FOLIO_BATCH_SIZE];
	int ret = 0;

	do {
		folio_batch_init(&fbatch);
		if (!shmem_find_swap_entries(mapping, start, &fbatch,
					     indices, type)) {
			ret = 0;
			break;
		}

		ret = shmem_unuse_swap_entries(inode, &fbatch, indices);
		if (ret < 0)
			break;

		start = indices[folio_batch_count(&fbatch) - 1];
	} while (true);

	return ret;
}

/*
 * Read all the shared memory data that resides in the swap
 * device 'type' back into memory, so the swap device can be
 * unused.
 */
int shmem_unuse(unsigned int type)
{
	struct shmem_inode_info *info, *next;
	int error = 0;

	if (list_empty(&shmem_swaplist))
		return 0;

	spin_lock(&shmem_swaplist_lock);
start_over:
	list_for_each_entry_safe(info, next, &shmem_swaplist, swaplist) {
		if (!info->swapped) {
			list_del_init(&info->swaplist);
			continue;
		}
		/*
		 * Drop the swaplist mutex while searching the inode for swap;
		 * but before doing so, make sure shmem_evict_inode() will not
		 * remove placeholder inode from swaplist, nor let it be freed
		 * (igrab() would protect from unlink, but not from unmount).
		 */
		atomic_inc(&info->stop_eviction);
		spin_unlock(&shmem_swaplist_lock);

		error = shmem_unuse_inode(&info->vfs_inode, type);
		cond_resched();

		spin_lock(&shmem_swaplist_lock);
		if (atomic_dec_and_test(&info->stop_eviction))
			wake_up_var(&info->stop_eviction);
		if (error)
			break;
		if (list_empty(&info->swaplist))
			goto start_over;
		next = list_next_entry(info, swaplist);
		if (!info->swapped)
			list_del_init(&info->swaplist);
	}
	spin_unlock(&shmem_swaplist_lock);

	return error;
}

/**
 * shmem_writeout - Write the folio to swap
 * @folio: The folio to write
 * @plug: swap plug
 * @folio_list: list to put back folios on split
 *
 * Move the folio from the page cache to the swap cache.
 */
int shmem_writeout(struct folio *folio, struct swap_iocb **plug,
		struct list_head *folio_list)
{
	struct address_space *mapping = folio->mapping;
	struct inode *inode = mapping->host;
	struct shmem_inode_info *info = SHMEM_I(inode);
	struct shmem_sb_info *sbinfo = SHMEM_SB(inode->i_sb);
	pgoff_t index;
	int nr_pages;
	bool split = false;

	if ((info->flags & SHMEM_F_LOCKED) || sbinfo->noswap)
		goto redirty;

	if (!total_swap_pages)
		goto redirty;

	/* Large folios must be split before swapping. */
	if (folio_test_large(folio))
		split = true;

	if (split) {
		int order;

try_split:
		order = folio_order(folio);
		/* Ensure the subpages are still dirty */
		folio_test_set_dirty(folio);
		if (split_folio_to_list(folio, folio_list))
			goto redirty;

		count_mthp_stat(order, MTHP_STAT_SWPOUT_FALLBACK);

		folio_clear_dirty(folio);
	}

	index = folio->index;
	nr_pages = folio_nr_pages(folio);

	/*
	 * This is somewhat ridiculous, but without plumbing a SWAP_MAP_FALLOC
	 * value into swapfile.c, the only way we can correctly account for a
	 * fallocated folio arriving here is now to initialize it and write it.
	 *
	 * That's okay for a folio already fallocated earlier, but if we have
	 * not yet completed the fallocation, then (a) we want to keep track
	 * of this folio in case we have to undo it, and (b) it may not be a
	 * good idea to continue anyway, once we're pushing into swap.  So
	 * reactivate the folio, and let shmem_fallocate() quit when too many.
	 */
	if (!folio_test_uptodate(folio)) {
		if (inode->i_private) {
			struct shmem_falloc *shmem_falloc;
			spin_lock(&inode->i_lock);
			shmem_falloc = inode->i_private;
			if (shmem_falloc &&
			    !shmem_falloc->waitq &&
			    index >= shmem_falloc->start &&
			    index < shmem_falloc->next)
				shmem_falloc->nr_unswapped += nr_pages;
			else
				shmem_falloc = NULL;
			spin_unlock(&inode->i_lock);
			if (shmem_falloc)
				goto redirty;
		}
		folio_zero_range(folio, 0, folio_size(folio));
		flush_dcache_folio(folio);
		folio_mark_uptodate(folio);
	}

	if (!folio_alloc_swap(folio)) {
		bool first_swapped = shmem_recalc_inode(inode, 0, nr_pages);
		int error;

		/*
		 * Add inode to shmem_unuse()'s list of swapped-out inodes,
		 * if it's not already there.  Do it now before the folio is
		 * removed from page cache, when its pagelock no longer
		 * protects the inode from eviction.  And do it now, after
		 * we've incremented swapped, because shmem_unuse() will
		 * prune a !swapped inode from the swaplist.
		 */
		if (first_swapped) {
			spin_lock(&shmem_swaplist_lock);
			if (list_empty(&info->swaplist))
				list_add(&info->swaplist, &shmem_swaplist);
			spin_unlock(&shmem_swaplist_lock);
		}

		folio_dup_swap(folio, NULL);
		shmem_delete_from_page_cache(folio, swp_to_radix_entry(folio->swap));

		BUG_ON(folio_mapped(folio));
		error = swap_writeout(folio, plug);
		if (error != AOP_WRITEPAGE_ACTIVATE) {
			/* folio has been unlocked */
			return error;
		}

		/*
		 * The intention here is to avoid holding on to the swap when
		 * zswap was unable to compress and unable to writeback; but
		 * it will be appropriate if other reactivate cases are added.
		 */
		error = shmem_add_to_page_cache(folio, mapping, index,
				swp_to_radix_entry(folio->swap),
				__GFP_HIGH | __GFP_NOMEMALLOC | __GFP_NOWARN);
		/* Swap entry might be erased by racing shmem_free_swap() */
		if (!error) {
			shmem_recalc_inode(inode, 0, -nr_pages);
			folio_put_swap(folio, NULL);
		}

		/*
		 * The swap_cache_del_folio() below could be left for
		 * shrink_folio_list()'s folio_free_swap() to dispose of;
		 * but I'm a little nervous about letting this folio out of
		 * shmem_writeout() in a hybrid half-tmpfs-half-swap state
		 * e.g. folio_mapping(folio) might give an unexpected answer.
		 */
		swap_cache_del_folio(folio);
		goto redirty;
	}
	if (nr_pages > 1)
		goto try_split;
redirty:
	folio_mark_dirty(folio);
	return AOP_WRITEPAGE_ACTIVATE;	/* Return with folio locked */
}
EXPORT_SYMBOL_GPL(shmem_writeout);

static inline void shmem_show_mpol(struct seq_file *seq, struct mempolicy *mpol)
{
}
static inline struct mempolicy *shmem_get_sbmpol(struct shmem_sb_info *sbinfo)
{
	return NULL;
}

static struct mempolicy *shmem_get_pgoff_policy(struct shmem_inode_info *info,
			pgoff_t index, unsigned int order, pgoff_t *ilx);

static struct folio *shmem_swapin_cluster(swp_entry_t swap, gfp_t gfp,
			struct shmem_inode_info *info, pgoff_t index)
{
	struct mempolicy *mpol;
	pgoff_t ilx;
	struct folio *folio;

	mpol = shmem_get_pgoff_policy(info, index, 0, &ilx);
	folio = swap_cluster_readahead(swap, gfp, mpol, ilx);
	mpol_cond_put(mpol);

	return folio;
}

/*
 * Make sure huge_gfp is always more limited than limit_gfp.
 * Some of the flags set permissions, while others set limitations.
 */
static gfp_t limit_gfp_mask(gfp_t huge_gfp, gfp_t limit_gfp)
{
	gfp_t allowflags = __GFP_IO | __GFP_FS | __GFP_RECLAIM;
	gfp_t denyflags = __GFP_NOWARN | __GFP_NORETRY;
	gfp_t zoneflags = limit_gfp & GFP_ZONEMASK;
	gfp_t result = huge_gfp & ~(allowflags | GFP_ZONEMASK);

	/* Allow allocations only from the originally specified zones. */
	result |= zoneflags;

	/*
	 * Minimize the result gfp by taking the union with the deny flags,
	 * and the intersection of the allow flags.
	 */
	result |= (limit_gfp & denyflags);
	result |= (huge_gfp & limit_gfp) & allowflags;

	return result;
}

static struct folio *shmem_alloc_folio(gfp_t gfp, int order,
		struct shmem_inode_info *info, pgoff_t index)
{
	struct mempolicy *mpol;
	pgoff_t ilx;
	struct folio *folio;

	mpol = shmem_get_pgoff_policy(info, index, order, &ilx);
	folio = folio_alloc_mpol(gfp, order, mpol, ilx, numa_node_id());
	mpol_cond_put(mpol);

	return folio;
}

static struct folio *shmem_alloc_and_add_folio(gfp_t gfp, struct inode *inode,
		pgoff_t index, struct mm_struct *fault_mm)
{
	struct address_space *mapping = inode->i_mapping;
	struct shmem_inode_info *info = SHMEM_I(inode);
	struct folio *folio;
	long pages = 1;
	int error;

	folio = shmem_alloc_folio(gfp, 0, info, index);
	if (!folio)
		return ERR_PTR(-ENOMEM);

	__folio_set_locked(folio);
	__folio_set_swapbacked(folio);

	gfp &= GFP_RECLAIM_MASK;
	error = mem_cgroup_charge(folio, fault_mm, gfp);
	if (error) {
		if (xa_find(&mapping->i_pages, &index,
				index + pages - 1, XA_PRESENT)) {
			error = -EEXIST;
		} else if (pages > 1) {
			if (pages == HPAGE_PMD_NR) {
				count_vm_event(THP_FILE_FALLBACK);
				count_vm_event(THP_FILE_FALLBACK_CHARGE);
			}
			count_mthp_stat(folio_order(folio), MTHP_STAT_SHMEM_FALLBACK);
			count_mthp_stat(folio_order(folio), MTHP_STAT_SHMEM_FALLBACK_CHARGE);
		}
		goto unlock;
	}

	error = shmem_add_to_page_cache(folio, mapping, index, NULL, gfp);
	if (error)
		goto unlock;

	error = shmem_inode_acct_blocks(inode, pages);
	if (error) {
		struct shmem_sb_info *sbinfo = SHMEM_SB(inode->i_sb);
		long freed;
		/*
		 * Try to reclaim some space by splitting a few
		 * large folios beyond i_size on the filesystem.
		 */
		shmem_unused_huge_shrink(sbinfo, NULL, pages);
		/*
		 * And do a shmem_recalc_inode() to account for freed pages:
		 * except our folio is there in cache, so not quite balanced.
		 */
		spin_lock(&info->lock);
		freed = pages + info->alloced - info->swapped -
			READ_ONCE(mapping->nrpages);
		if (freed > 0)
			info->alloced -= freed;
		spin_unlock(&info->lock);
		if (freed > 0)
			shmem_inode_unacct_blocks(inode, freed);
		error = shmem_inode_acct_blocks(inode, pages);
		if (error) {
			filemap_remove_folio(folio);
			goto unlock;
		}
	}

	shmem_recalc_inode(inode, pages, 0);
	folio_add_lru(folio);
	return folio;

unlock:
	folio_unlock(folio);
	folio_put(folio);
	return ERR_PTR(error);
}

static struct folio *shmem_swap_alloc_folio(struct inode *inode,
		struct vm_area_struct *vma, pgoff_t index,
		swp_entry_t entry, int order, gfp_t gfp)
{
	struct shmem_inode_info *info = SHMEM_I(inode);
	struct folio *new, *swapcache;

	if (WARN_ON_ONCE(order))
		return ERR_PTR(-EINVAL);

	new = shmem_alloc_folio(gfp, order, info, index);
	if (!new)
		return ERR_PTR(-ENOMEM);

	if (mem_cgroup_swapin_charge_folio(new, vma ? vma->vm_mm : NULL,
					   gfp, entry)) {
		folio_put(new);
		return ERR_PTR(-ENOMEM);
	}

	swapcache = swapin_folio(entry, new);
	if (swapcache != new) {
		folio_put(new);
		if (!swapcache) {
			/*
			 * The new folio is charged already, swapin can
			 * only fail due to another raced swapin.
			 */
			return ERR_PTR(-EEXIST);
		}
	}
	return swapcache;
}

/*
 * When a page is moved from swapcache to shmem filecache (either by the
 * usual swapin of shmem_get_folio_gfp(), or by the less common swapoff of
 * shmem_unuse_inode()), it may have been read in earlier from swap, in
 * ignorance of the mapping it belongs to.  If that mapping has special
 * constraints (like the gma500 GEM driver, which requires RAM below 4GB),
 * we may need to copy to a suitable page before moving to filecache.
 *
 * In a future release, this may well be extended to respect cpuset and
 * NUMA mempolicy, and applied also to anonymous pages in do_swap_page();
 * but for now it is a simple matter of zone.
 */
static bool shmem_should_replace_folio(struct folio *folio, gfp_t gfp)
{
	return folio_zonenum(folio) > gfp_zone(gfp);
}

static int shmem_replace_folio(struct folio **foliop, gfp_t gfp,
				struct shmem_inode_info *info, pgoff_t index,
				struct vm_area_struct *vma)
{
	struct swap_cluster_info *ci;
	struct folio *new, *old = *foliop;
	swp_entry_t entry = old->swap;
	int nr_pages = folio_nr_pages(old);
	int error = 0;

	/*
	 * We have arrived here because our zones are constrained, so don't
	 * limit chance of success by further cpuset and node constraints.
	 */
	gfp &= ~GFP_CONSTRAINT_MASK;

	new = shmem_alloc_folio(gfp, folio_order(old), info, index);
	if (!new)
		return -ENOMEM;

	folio_ref_add(new, nr_pages);
	folio_copy(new, old);
	flush_dcache_folio(new);

	__folio_set_locked(new);
	__folio_set_swapbacked(new);
	folio_mark_uptodate(new);
	new->swap = entry;
	folio_set_swapcache(new);

	ci = swap_cluster_get_and_lock_irq(old);
	__swap_cache_replace_folio(ci, old, new);
	mem_cgroup_replace_folio(old, new);
	shmem_update_stats(new, nr_pages);
	shmem_update_stats(old, -nr_pages);
	swap_cluster_unlock_irq(ci);

	folio_add_lru(new);
	*foliop = new;

	folio_clear_swapcache(old);
	old->private = NULL;

	folio_unlock(old);
	/*
	 * The old folio are removed from swap cache, drop the 'nr_pages'
	 * reference, as well as one temporary reference getting from swap
	 * cache.
	 */
	folio_put_refs(old, nr_pages + 1);
	return error;
}

static void shmem_set_folio_swapin_error(struct inode *inode, pgoff_t index,
					 struct folio *folio, swp_entry_t swap)
{
	struct address_space *mapping = inode->i_mapping;
	swp_entry_t swapin_error;
	void *old;
	int nr_pages;

	swapin_error = make_poisoned_swp_entry();
	old = xa_cmpxchg_irq(&mapping->i_pages, index,
			     swp_to_radix_entry(swap),
			     swp_to_radix_entry(swapin_error), 0);
	if (old != swp_to_radix_entry(swap))
		return;

	nr_pages = folio_nr_pages(folio);
	folio_wait_writeback(folio);
	folio_put_swap(folio, NULL);
	swap_cache_del_folio(folio);
	/*
	 * Don't treat swapin error folio as alloced. Otherwise inode->i_blocks
	 * won't be 0 when inode is released and thus trigger WARN_ON(i_blocks)
	 * in shmem_evict_inode().
	 */
	shmem_recalc_inode(inode, -nr_pages, -nr_pages);
}

static int shmem_split_large_entry(struct inode *inode, pgoff_t index,
				   swp_entry_t swap, gfp_t gfp)
{
	struct address_space *mapping = inode->i_mapping;
	XA_STATE_ORDER(xas, &mapping->i_pages, index, 0);
	int split_order = 0;
	int i;

	/* Convert user data gfp flags to xarray node gfp flags */
	gfp &= GFP_RECLAIM_MASK;

	for (;;) {
		void *old = NULL;
		int cur_order;
		pgoff_t swap_index;

		xas_lock_irq(&xas);
		old = xas_load(&xas);
		if (!xa_is_value(old) || swp_to_radix_entry(swap) != old) {
			xas_set_err(&xas, -EEXIST);
			goto unlock;
		}

		cur_order = xas_get_order(&xas);
		if (!cur_order)
			goto unlock;

		/* Try to split large swap entry in pagecache */
		swap_index = round_down(index, 1 << cur_order);
		split_order = xas_try_split_min_order(cur_order);

		while (cur_order > 0) {
			pgoff_t aligned_index =
				round_down(index, 1 << cur_order);
			pgoff_t swap_offset = aligned_index - swap_index;

			xas_set_order(&xas, index, split_order);
			xas_try_split(&xas, old, cur_order);
			if (xas_error(&xas))
				goto unlock;

			/*
			 * Re-set the swap entry after splitting, and the swap
			 * offset of the original large entry must be continuous.
			 */
			for (i = 0; i < 1 << cur_order;
			     i += (1 << split_order)) {
				swp_entry_t tmp;

				tmp = swp_entry(swp_type(swap),
						swp_offset(swap) + swap_offset +
							i);
				__xa_store(&mapping->i_pages, aligned_index + i,
					   swp_to_radix_entry(tmp), 0);
			}
			cur_order = split_order;
			split_order = xas_try_split_min_order(split_order);
		}

unlock:
		xas_unlock_irq(&xas);

		if (!xas_nomem(&xas, gfp))
			break;
	}

	if (xas_error(&xas))
		return xas_error(&xas);

	return 0;
}

/*
 * Swap in the folio pointed to by *foliop.
 * Caller has to make sure that *foliop contains a valid swapped folio.
 * Returns 0 and the folio in foliop if success. On failure, returns the
 * error code and NULL in *foliop.
 */
static int shmem_swapin_folio(struct inode *inode, pgoff_t index,
			     struct folio **foliop, enum sgp_type sgp,
			     gfp_t gfp, struct vm_area_struct *vma,
			     vm_fault_t *fault_type)
{
	struct address_space *mapping = inode->i_mapping;
	struct mm_struct *fault_mm = vma ? vma->vm_mm : NULL;
	struct shmem_inode_info *info = SHMEM_I(inode);
	swp_entry_t swap;
	softleaf_t index_entry;
	struct swap_info_struct *si;
	struct folio *folio = NULL;
	int error, nr_pages, order;
	pgoff_t offset;

	VM_BUG_ON(!*foliop || !xa_is_value(*foliop));
	index_entry = radix_to_swp_entry(*foliop);
	swap = index_entry;
	*foliop = NULL;

	if (softleaf_is_poison_marker(index_entry))
		return -EIO;

	si = get_swap_device(index_entry);
	order = shmem_confirm_swap(mapping, index, index_entry);
	if (unlikely(!si)) {
		if (order < 0)
			return -EEXIST;
		else
			return -EINVAL;
	}
	if (unlikely(order < 0)) {
		put_swap_device(si);
		return -EEXIST;
	}

	/* index may point to the middle of a large entry, get the sub entry */
	if (order) {
		offset = index - round_down(index, 1 << order);
		swap = swp_entry(swp_type(swap), swp_offset(swap) + offset);
	}

	/* Look it up and read it in.. */
	folio = swap_cache_get_folio(swap);
	if (!folio) {
		if (data_race(si->flags & SWP_SYNCHRONOUS_IO)) {
			/* Direct swapin skipping swap cache & readahead */
			folio = shmem_swap_alloc_folio(inode, vma, index,
						       index_entry, order, gfp);
			if (IS_ERR(folio)) {
				error = PTR_ERR(folio);
				folio = NULL;
				goto failed;
			}
		} else {
			/* Cached swapin only supports order 0 folio */
			folio = shmem_swapin_cluster(swap, gfp, info, index);
			if (!folio) {
				error = -ENOMEM;
				goto failed;
			}
		}
		if (fault_type) {
			*fault_type |= VM_FAULT_MAJOR;
			count_vm_event(PGMAJFAULT);
			count_memcg_event_mm(fault_mm, PGMAJFAULT);
		}
	} else {
		swap_update_readahead(folio, NULL, 0);
	}

	if (order > folio_order(folio)) {
		/*
		 * Swapin may get smaller folios due to various reasons:
		 * It may fallback to order 0 due to memory pressure or race,
		 * swap readahead may swap in order 0 folios into swapcache
		 * asynchronously, while the shmem mapping can still stores
		 * large swap entries. In such cases, we should split the
		 * large swap entry to prevent possible data corruption.
		 */
		error = shmem_split_large_entry(inode, index, index_entry, gfp);
		if (error)
			goto failed_nolock;
	}

	/*
	 * If the folio is large, round down swap and index by folio size.
	 * No matter what race occurs, the swap layer ensures we either get
	 * a valid folio that has its swap entry aligned by size, or a
	 * temporarily invalid one which we'll abort very soon and retry.
	 *
	 * shmem_add_to_page_cache ensures the whole range contains expected
	 * entries and prevents any corruption, so any race split is fine
	 * too, it will succeed as long as the entries are still there.
	 */
	nr_pages = folio_nr_pages(folio);
	if (nr_pages > 1) {
		swap.val = round_down(swap.val, nr_pages);
		index = round_down(index, nr_pages);
	}

	/*
	 * We have to do this with the folio locked to prevent races.
	 * The shmem_confirm_swap below only checks if the first swap
	 * entry matches the folio, that's enough to ensure the folio
	 * is not used outside of shmem, as shmem swap entries
	 * and swap cache folios are never partially freed.
	 */
	folio_lock(folio);
	if (!folio_matches_swap_entry(folio, swap) ||
	    shmem_confirm_swap(mapping, index, swap) < 0) {
		error = -EEXIST;
		goto unlock;
	}
	if (!folio_test_uptodate(folio)) {
		error = -EIO;
		goto failed;
	}
	folio_wait_writeback(folio);

	/*
	 * Some architectures may have to restore extra metadata to the
	 * folio after reading from swap.
	 */
	arch_swap_restore(folio_swap(swap, folio), folio);

	if (shmem_should_replace_folio(folio, gfp)) {
		error = shmem_replace_folio(&folio, gfp, info, index, vma);
		if (error)
			goto failed;
	}

	error = shmem_add_to_page_cache(folio, mapping, index,
					swp_to_radix_entry(swap), gfp);
	if (error)
		goto failed;

	shmem_recalc_inode(inode, 0, -nr_pages);

	if (sgp == SGP_WRITE)
		folio_mark_accessed(folio);

	folio_put_swap(folio, NULL);
	swap_cache_del_folio(folio);
	folio_mark_dirty(folio);
	put_swap_device(si);

	*foliop = folio;
	return 0;
failed:
	if (shmem_confirm_swap(mapping, index, swap) < 0)
		error = -EEXIST;
	if (error == -EIO)
		shmem_set_folio_swapin_error(inode, index, folio, swap);
unlock:
	if (folio)
		folio_unlock(folio);
failed_nolock:
	if (folio)
		folio_put(folio);
	put_swap_device(si);

	return error;
}

/*
 * shmem_get_folio_gfp - find page in cache, or get from swap, or allocate
 *
 * If we allocate a new one we do not mark it dirty. That's up to the
 * vm. If we swap it in we mark it dirty since we also free the swap
 * entry since a page cannot live in both the swap and page cache.
 *
 * vmf and fault_type are only supplied by shmem_fault: otherwise they are NULL.
 */
static int shmem_get_folio_gfp(struct inode *inode, pgoff_t index,
		loff_t write_end, struct folio **foliop, enum sgp_type sgp,
		gfp_t gfp, struct vm_fault *vmf, vm_fault_t *fault_type)
{
	struct vm_area_struct *vma = vmf ? vmf->vma : NULL;
	struct mm_struct *fault_mm;
	struct folio *folio;
	int error;
	bool alloced;
	unsigned long orders = 0;

	if (WARN_ON_ONCE(!shmem_mapping(inode->i_mapping)))
		return -EINVAL;

	if (index > (MAX_LFS_FILESIZE >> PAGE_SHIFT))
		return -EFBIG;
repeat:
	if (sgp <= SGP_CACHE &&
	    ((loff_t)index << PAGE_SHIFT) >= i_size_read(inode))
		return -EINVAL;

	alloced = false;
	fault_mm = vma ? vma->vm_mm : NULL;

	folio = filemap_get_entry(inode->i_mapping, index);
	if (folio && vma && userfaultfd_minor(vma)) {
		if (!xa_is_value(folio))
			folio_put(folio);
		*fault_type = handle_userfault(vmf, VM_UFFD_MINOR);
		return 0;
	}

	if (xa_is_value(folio)) {
		error = shmem_swapin_folio(inode, index, &folio,
					   sgp, gfp, vma, fault_type);
		if (error == -EEXIST)
			goto repeat;

		*foliop = folio;
		return error;
	}

	if (folio) {
		folio_lock(folio);

		/* Has the folio been truncated or swapped out? */
		if (unlikely(folio->mapping != inode->i_mapping)) {
			folio_unlock(folio);
			folio_put(folio);
			goto repeat;
		}
		if (sgp == SGP_WRITE)
			folio_mark_accessed(folio);
		if (folio_test_uptodate(folio))
			goto out;
		/* fallocated folio */
		if (sgp != SGP_READ)
			goto clear;
		folio_unlock(folio);
		folio_put(folio);
	}

	/*
	 * SGP_READ: succeed on hole, with NULL folio, letting caller zero.
	 * SGP_NOALLOC: fail on hole, with NULL folio, letting caller fail.
	 */
	*foliop = NULL;
	if (sgp == SGP_READ)
		return 0;
	if (sgp == SGP_NOALLOC)
		return -ENOENT;

	/*
	 * Fast cache lookup and swap lookup did not find it: allocate.
	 */

	if (vma && userfaultfd_missing(vma)) {
		*fault_type = handle_userfault(vmf, VM_UFFD_MISSING);
		return 0;
	}

	/* Find hugepage orders that are allowed for anonymous shmem and tmpfs. */
	orders = shmem_allowable_huge_orders(inode, vma, index, write_end, false);
	if (orders > 0) {
		gfp_t huge_gfp;

		huge_gfp = vma_thp_gfp_mask(vma);
		huge_gfp = limit_gfp_mask(huge_gfp, gfp);
		folio = shmem_alloc_and_add_folio(huge_gfp, inode, index,
						 fault_mm);
		if (!IS_ERR(folio)) {
			if (folio_test_pmd_mappable(folio))
				count_vm_event(THP_FILE_ALLOC);
			count_mthp_stat(folio_order(folio), MTHP_STAT_SHMEM_ALLOC);
			goto alloced;
		}
		if (PTR_ERR(folio) == -EEXIST)
			goto repeat;
	}

	folio = shmem_alloc_and_add_folio(gfp, inode, index, fault_mm);
	if (IS_ERR(folio)) {
		error = PTR_ERR(folio);
		if (error == -EEXIST)
			goto repeat;
		folio = NULL;
		goto unlock;
	}

alloced:
	alloced = true;
	if (folio_test_large(folio) &&
	    DIV_ROUND_UP(i_size_read(inode), PAGE_SIZE) <
					folio_next_index(folio)) {
		struct shmem_sb_info *sbinfo = SHMEM_SB(inode->i_sb);
		struct shmem_inode_info *info = SHMEM_I(inode);
		/*
		 * Part of the large folio is beyond i_size: subject
		 * to shrink under memory pressure.
		 */
		spin_lock(&sbinfo->shrinklist_lock);
		/*
		 * _careful to defend against unlocked access to
		 * ->shrink_list in shmem_unused_huge_shrink()
		 */
		if (list_empty_careful(&info->shrinklist)) {
			list_add_tail(&info->shrinklist,
				      &sbinfo->shrinklist);
			sbinfo->shrinklist_len++;
		}
		spin_unlock(&sbinfo->shrinklist_lock);
	}

	if (sgp == SGP_WRITE)
		folio_set_referenced(folio);
	/*
	 * Let SGP_FALLOC use the SGP_WRITE optimization on a new folio.
	 */
	if (sgp == SGP_FALLOC)
		sgp = SGP_WRITE;
clear:
	/*
	 * Let SGP_WRITE caller clear ends if write does not fill folio;
	 * but SGP_FALLOC on a folio fallocated earlier must initialize
	 * it now, lest undo on failure cancel our earlier guarantee.
	 */
	if (sgp != SGP_WRITE && !folio_test_uptodate(folio)) {
		long i, n = folio_nr_pages(folio);

		for (i = 0; i < n; i++)
			clear_highpage(folio_page(folio, i));
		flush_dcache_folio(folio);
		folio_mark_uptodate(folio);
	}

	/* Perhaps the file has been truncated since we checked */
	if (sgp <= SGP_CACHE &&
	    ((loff_t)index << PAGE_SHIFT) >= i_size_read(inode)) {
		error = -EINVAL;
		goto unlock;
	}
out:
	*foliop = folio;
	return 0;

	/*
	 * Error recovery.
	 */
unlock:
	if (alloced)
		filemap_remove_folio(folio);
	shmem_recalc_inode(inode, 0, 0);
	if (folio) {
		folio_unlock(folio);
		folio_put(folio);
	}
	return error;
}

/**
 * shmem_get_folio - find, and lock a shmem folio.
 * @inode:	inode to search
 * @index:	the page index.
 * @write_end:	end of a write, could extend inode size
 * @foliop:	pointer to the folio if found
 * @sgp:	SGP_* flags to control behavior
 *
 * Looks up the page cache entry at @inode & @index.  If a folio is
 * present, it is returned locked with an increased refcount.
 *
 * If the caller modifies data in the folio, it must call folio_mark_dirty()
 * before unlocking the folio to ensure that the folio is not reclaimed.
 * There is no need to reserve space before calling folio_mark_dirty().
 *
 * When no folio is found, the behavior depends on @sgp:
 *  - for SGP_READ, *@foliop is %NULL and 0 is returned
 *  - for SGP_NOALLOC, *@foliop is %NULL and -ENOENT is returned
 *  - for all other flags a new folio is allocated, inserted into the
 *    page cache and returned locked in @foliop.
 *
 * Context: May sleep.
 * Return: 0 if successful, else a negative error code.
 */
int shmem_get_folio(struct inode *inode, pgoff_t index, loff_t write_end,
		    struct folio **foliop, enum sgp_type sgp)
{
	return shmem_get_folio_gfp(inode, index, write_end, foliop, sgp,
			mapping_gfp_mask(inode->i_mapping), NULL, NULL);
}
EXPORT_SYMBOL_GPL(shmem_get_folio);

/*
 * This is like autoremove_wake_function, but it removes the wait queue
 * entry unconditionally - even if something else had already woken the
 * target.
 */
static int synchronous_wake_function(wait_queue_entry_t *wait,
			unsigned int mode, int sync, void *key)
{
	int ret = default_wake_function(wait, mode, sync, key);
	list_del_init(&wait->entry);
	return ret;
}

/*
 * Trinity finds that probing a hole which tmpfs is punching can
 * prevent the hole-punch from ever completing: which in turn
 * locks writers out with its hold on i_rwsem.  So refrain from
 * faulting pages into the hole while it's being punched.  Although
 * shmem_undo_range() does remove the additions, it may be unable to
 * keep up, as each new page needs its own unmap_mapping_range() call,
 * and the i_mmap tree grows ever slower to scan if new vmas are added.
 *
 * It does not matter if we sometimes reach this check just before the
 * hole-punch begins, so that one fault then races with the punch:
 * we just need to make racing faults a rare case.
 *
 * The implementation below would be much simpler if we just used a
 * standard mutex or completion: but we cannot take i_rwsem in fault,
 * and bloating every shmem inode for this unlikely case would be sad.
 */
static vm_fault_t shmem_falloc_wait(struct vm_fault *vmf, struct inode *inode)
{
	struct shmem_falloc *shmem_falloc;
	struct file *fpin = NULL;
	vm_fault_t ret = 0;

	spin_lock(&inode->i_lock);
	shmem_falloc = inode->i_private;
	if (shmem_falloc &&
	    shmem_falloc->waitq &&
	    vmf->pgoff >= shmem_falloc->start &&
	    vmf->pgoff < shmem_falloc->next) {
		wait_queue_head_t *shmem_falloc_waitq;
		DEFINE_WAIT_FUNC(shmem_fault_wait, synchronous_wake_function);

		ret = VM_FAULT_NOPAGE;
		fpin = maybe_unlock_mmap_for_io(vmf, NULL);
		shmem_falloc_waitq = shmem_falloc->waitq;
		prepare_to_wait(shmem_falloc_waitq, &shmem_fault_wait,
				TASK_UNINTERRUPTIBLE);
		spin_unlock(&inode->i_lock);
		schedule();

		/*
		 * shmem_falloc_waitq points into the shmem_fallocate()
		 * stack of the hole-punching task: shmem_falloc_waitq
		 * is usually invalid by the time we reach here, but
		 * finish_wait() does not dereference it in that case;
		 * though i_lock needed lest racing with wake_up_all().
		 */
		spin_lock(&inode->i_lock);
		finish_wait(shmem_falloc_waitq, &shmem_fault_wait);
	}
	spin_unlock(&inode->i_lock);
	if (fpin) {
		fput(fpin);
		ret = VM_FAULT_RETRY;
	}
	return ret;
}

static vm_fault_t shmem_fault(struct vm_fault *vmf)
{
	struct inode *inode = file_inode(vmf->vma->vm_file);
	gfp_t gfp = mapping_gfp_mask(inode->i_mapping);
	struct folio *folio = NULL;
	vm_fault_t ret = 0;
	int err;

	/*
	 * Trinity finds that probing a hole which tmpfs is punching can
	 * prevent the hole-punch from ever completing: noted in i_private.
	 */
	if (unlikely(inode->i_private)) {
		ret = shmem_falloc_wait(vmf, inode);
		if (ret)
			return ret;
	}

	WARN_ON_ONCE(vmf->page != NULL);
	err = shmem_get_folio_gfp(inode, vmf->pgoff, 0, &folio, SGP_CACHE,
				  gfp, vmf, &ret);
	if (err)
		return vmf_error(err);
	if (folio) {
		vmf->page = folio_file_page(folio, vmf->pgoff);
		ret |= VM_FAULT_LOCKED;
	}
	return ret;
}

unsigned long shmem_get_unmapped_area(struct file *file,
				      unsigned long uaddr, unsigned long len,
				      unsigned long pgoff, unsigned long flags)
{
	unsigned long addr;

	if (len > TASK_SIZE)
		return -ENOMEM;

	addr = mm_get_unmapped_area(file, uaddr, len, pgoff, flags);
	return addr;
}

#ifdef CONFIG_NUMA
static int shmem_set_policy(struct vm_area_struct *vma, struct mempolicy *mpol)
{
	struct inode *inode = file_inode(vma->vm_file);
	return mpol_set_shared_policy(&SHMEM_I(inode)->policy, vma, mpol);
}

static struct mempolicy *shmem_get_policy(struct vm_area_struct *vma,
					  unsigned long addr, pgoff_t *ilx)
{
	struct inode *inode = file_inode(vma->vm_file);
	pgoff_t index;

	/*
	 * Bias interleave by inode number to distribute better across nodes;
	 * but this interface is independent of which page order is used, so
	 * supplies only that bias, letting caller apply the offset (adjusted
	 * by page order, as in shmem_get_pgoff_policy() and get_vma_policy()).
	 */
	*ilx = inode->i_ino;
	index = ((addr - vma->vm_start) >> PAGE_SHIFT) + vma->vm_pgoff;
	return mpol_shared_policy_lookup(&SHMEM_I(inode)->policy, index);
}

static struct mempolicy *shmem_get_pgoff_policy(struct shmem_inode_info *info,
			pgoff_t index, unsigned int order, pgoff_t *ilx)
{
	struct mempolicy *mpol;

	/* Bias interleave by inode number to distribute better across nodes */
	*ilx = info->vfs_inode.i_ino + (index >> order);

	mpol = mpol_shared_policy_lookup(&info->policy, index);
	return mpol ? mpol : get_task_policy(current);
}
#else
static struct mempolicy *shmem_get_pgoff_policy(struct shmem_inode_info *info,
			pgoff_t index, unsigned int order, pgoff_t *ilx)
{
	*ilx = 0;
	return NULL;
}
#endif /* CONFIG_NUMA */

int shmem_lock(struct file *file, int lock, struct ucounts *ucounts)
{
	struct inode *inode = file_inode(file);
	struct shmem_inode_info *info = SHMEM_I(inode);
	int retval = -ENOMEM;

	/*
	 * What serializes the accesses to info->flags?
	 * ipc_lock_object() when called from shmctl_do_lock(),
	 * no serialization needed when called from shm_destroy().
	 */
	if (lock && !(info->flags & SHMEM_F_LOCKED)) {
		if (!user_shm_lock(inode->i_size, ucounts))
			goto out_nomem;
		info->flags |= SHMEM_F_LOCKED;
		mapping_set_unevictable(file->f_mapping);
	}
	if (!lock && (info->flags & SHMEM_F_LOCKED) && ucounts) {
		user_shm_unlock(inode->i_size, ucounts);
		info->flags &= ~SHMEM_F_LOCKED;
		mapping_clear_unevictable(file->f_mapping);
	}
	retval = 0;

out_nomem:
	return retval;
}

static int shmem_mmap_prepare(struct vm_area_desc *desc)
{
	struct file *file = desc->file;
	struct inode *inode = file_inode(file);

	file_accessed(file);
	/* This is anonymous shared memory if it is unlinked at the time of mmap */
	if (inode->i_nlink)
		desc->vm_ops = &shmem_vm_ops;
	else
		desc->vm_ops = &shmem_anon_vm_ops;
	return 0;
}

static int shmem_file_open(struct inode *inode, struct file *file)
{
	file->f_mode |= FMODE_CAN_ODIRECT;
	return generic_file_open(inode, file);
}

static void shmem_set_inode_flags(struct inode *inode, unsigned int fsflags, struct dentry *dentry)
{
}
#define shmem_initxattrs NULL

static struct offset_ctx *shmem_get_offset_ctx(struct inode *inode)
{
	return &SHMEM_I(inode)->dir_offsets;
}

static struct inode *__shmem_get_inode(struct mnt_idmap *idmap,
				       struct super_block *sb,
				       struct inode *dir, umode_t mode,
				       dev_t dev, vma_flags_t flags)
{
	struct inode *inode;
	struct shmem_inode_info *info;
	struct shmem_sb_info *sbinfo = SHMEM_SB(sb);
	ino_t ino;
	int err;

	err = shmem_reserve_inode(sb, &ino);
	if (err)
		return ERR_PTR(err);

	inode = new_inode(sb);
	if (!inode) {
		shmem_free_inode(sb, 0);
		return ERR_PTR(-ENOSPC);
	}

	inode->i_ino = ino;
	inode_init_owner(idmap, inode, dir, mode);
	inode->i_blocks = 0;
	simple_inode_init_ts(inode);
	inode->i_generation = get_random_u32();
	info = SHMEM_I(inode);
	memset(info, 0, (char *)inode - (char *)info);
	spin_lock_init(&info->lock);
	atomic_set(&info->stop_eviction, 0);
	info->seals = F_SEAL_SEAL;
	info->flags = vma_flags_test(&flags, VMA_NORESERVE_BIT)
		? SHMEM_F_NORESERVE : 0;
	info->i_crtime = inode_get_mtime(inode);
	info->fsflags = (dir == NULL) ? 0 :
		SHMEM_I(dir)->fsflags & SHMEM_FL_INHERITED;
	if (info->fsflags)
		shmem_set_inode_flags(inode, info->fsflags, NULL);
	INIT_LIST_HEAD(&info->shrinklist);
	INIT_LIST_HEAD(&info->swaplist);
	if (sbinfo->noswap)
		mapping_set_unevictable(inode->i_mapping);

	/* Don't consider 'deny' for emergencies and 'force' for testing */
	if (sbinfo->huge)
		mapping_set_large_folios(inode->i_mapping);

	switch (mode & S_IFMT) {
	default:
		inode->i_op = &shmem_special_inode_operations;
		init_special_inode(inode, mode, dev);
		break;
	case S_IFREG:
		inode->i_mapping->a_ops = &shmem_aops;
		inode->i_op = &shmem_inode_operations;
		inode->i_fop = &shmem_file_operations;
		mpol_shared_policy_init(&info->policy,
					 shmem_get_sbmpol(sbinfo));
		break;
	case S_IFDIR:
		inc_nlink(inode);
		/* Some things misbehave if size == 0 on a directory */
		inode->i_size = 2 * BOGO_DIRENT_SIZE;
		inode->i_op = &shmem_dir_inode_operations;
		inode->i_fop = &simple_offset_dir_operations;
		simple_offset_init(shmem_get_offset_ctx(inode));
		break;
	case S_IFLNK:
		/*
		 * Must not load anything in the rbtree,
		 * mpol_free_shared_policy will not be called.
		 */
		mpol_shared_policy_init(&info->policy, NULL);
		break;
	}

	lockdep_annotate_inode_mutex_key(inode);
	return inode;
}

static struct inode *shmem_get_inode(struct mnt_idmap *idmap,
				     struct super_block *sb, struct inode *dir,
				     umode_t mode, dev_t dev, vma_flags_t flags)
{
	return __shmem_get_inode(idmap, sb, dir, mode, dev, flags);
}



static void shmem_put_super(struct super_block *sb)
{
	struct shmem_sb_info *sbinfo = SHMEM_SB(sb);


	free_percpu(sbinfo->ino_batch);
	percpu_counter_destroy(&sbinfo->used_blocks);
	mpol_put(sbinfo->mpol);
	kfree(sbinfo);
	sb->s_fs_info = NULL;
}


static int shmem_fill_super(struct super_block *sb, struct fs_context *fc)
{
	struct shmem_options *ctx = fc->fs_private;
	struct inode *inode;
	struct shmem_sb_info *sbinfo;
	int error = -ENOMEM;

	/* Round up to L1_CACHE_BYTES to resist false sharing */
	sbinfo = kzalloc(max((int)sizeof(struct shmem_sb_info),
				L1_CACHE_BYTES), GFP_KERNEL);
	if (!sbinfo)
		return error;

	sb->s_fs_info = sbinfo;

	sb->s_flags |= SB_NOUSER;
	sb->s_d_flags |= DCACHE_DONTCACHE;
	sbinfo->max_blocks = ctx->blocks;
	sbinfo->max_inodes = ctx->inodes;
	sbinfo->free_ispace = sbinfo->max_inodes * BOGO_INODE_SIZE;
	if (sb->s_flags & SB_KERNMOUNT) {
		sbinfo->ino_batch = alloc_percpu(ino_t);
		if (!sbinfo->ino_batch)
			goto failed;
	}
	sbinfo->uid = ctx->uid;
	sbinfo->gid = ctx->gid;
	sbinfo->full_inums = ctx->full_inums;
	sbinfo->mode = ctx->mode;
	sbinfo->mpol = ctx->mpol;
	ctx->mpol = NULL;

	raw_spin_lock_init(&sbinfo->stat_lock);
	if (percpu_counter_init(&sbinfo->used_blocks, 0, GFP_KERNEL))
		goto failed;
	spin_lock_init(&sbinfo->shrinklist_lock);
	INIT_LIST_HEAD(&sbinfo->shrinklist);

	sb->s_maxbytes = MAX_LFS_FILESIZE;
	sb->s_blocksize = PAGE_SIZE;
	sb->s_blocksize_bits = PAGE_SHIFT;
	sb->s_magic = TMPFS_MAGIC;
	sb->s_op = &shmem_ops;
	sb->s_time_gran = 1;
	uuid_t uuid;
	uuid_gen(&uuid);
	super_set_uuid(sb, uuid.b, sizeof(uuid));


	inode = shmem_get_inode(&nop_mnt_idmap, sb, NULL,
				S_IFDIR | sbinfo->mode, 0,
				mk_vma_flags(VMA_NORESERVE_BIT));
	if (IS_ERR(inode)) {
		error = PTR_ERR(inode);
		goto failed;
	}
	inode->i_uid = sbinfo->uid;
	inode->i_gid = sbinfo->gid;
	sb->s_root = d_make_root(inode);
	if (!sb->s_root)
		goto failed;
	return 0;

failed:
	shmem_put_super(sb);
	return error;
}

static int shmem_get_tree(struct fs_context *fc)
{
	return get_tree_nodev(fc, shmem_fill_super);
}

static void shmem_free_fc(struct fs_context *fc)
{
	struct shmem_options *ctx = fc->fs_private;

	if (ctx) {
		mpol_put(ctx->mpol);
		kfree(ctx);
	}
}

static const struct fs_context_operations shmem_fs_context_ops = {
	.free			= shmem_free_fc,
	.get_tree		= shmem_get_tree,
};

static struct kmem_cache *shmem_inode_cachep __ro_after_init;

static struct inode *shmem_alloc_inode(struct super_block *sb)
{
	struct shmem_inode_info *info;
	info = alloc_inode_sb(sb, shmem_inode_cachep, GFP_KERNEL);
	if (!info)
		return NULL;
	return &info->vfs_inode;
}

static void shmem_free_in_core_inode(struct inode *inode)
{
	if (S_ISLNK(inode->i_mode))
		kfree(inode->i_link);
	kmem_cache_free(shmem_inode_cachep, SHMEM_I(inode));
}

static void shmem_destroy_inode(struct inode *inode)
{
	if (S_ISREG(inode->i_mode))
		mpol_free_shared_policy(&SHMEM_I(inode)->policy);
	if (S_ISDIR(inode->i_mode))
		simple_offset_destroy(shmem_get_offset_ctx(inode));
}

static void shmem_init_inode(void *foo)
{
	struct shmem_inode_info *info = foo;
	inode_init_once(&info->vfs_inode);
}

static void __init shmem_init_inodecache(void)
{
	shmem_inode_cachep = kmem_cache_create("shmem_inode_cache",
				sizeof(struct shmem_inode_info),
				0, SLAB_PANIC|SLAB_ACCOUNT, shmem_init_inode);
}

static void __init shmem_destroy_inodecache(void)
{
	kmem_cache_destroy(shmem_inode_cachep);
}

/* Keep the page in page cache instead of truncating it */
static int shmem_error_remove_folio(struct address_space *mapping,
				   struct folio *folio)
{
	return 0;
}

static const struct address_space_operations shmem_aops = {
	.dirty_folio	= noop_dirty_folio,
#ifdef CONFIG_MIGRATION
	.migrate_folio	= migrate_folio,
#endif
	.error_remove_folio = shmem_error_remove_folio,
};

static const struct file_operations shmem_file_operations = {
	.mmap_prepare	= shmem_mmap_prepare,
	.open		= shmem_file_open,
	.get_unmapped_area = shmem_get_unmapped_area,
};

static const struct inode_operations shmem_inode_operations = {
	.getattr	= shmem_getattr,
	.setattr	= shmem_setattr,
};

static const struct inode_operations shmem_dir_inode_operations = {
};

static const struct inode_operations shmem_special_inode_operations = {
	.getattr	= shmem_getattr,
};

static const struct super_operations shmem_ops = {
	.alloc_inode	= shmem_alloc_inode,
	.free_inode	= shmem_free_in_core_inode,
	.destroy_inode	= shmem_destroy_inode,
	.evict_inode	= shmem_evict_inode,
	.drop_inode	= inode_just_drop,
	.put_super	= shmem_put_super,
};

static const struct vm_operations_struct shmem_vm_ops = {
	.fault		= shmem_fault,
	.map_pages	= filemap_map_pages,
#ifdef CONFIG_NUMA
	.set_policy     = shmem_set_policy,
	.get_policy     = shmem_get_policy,
#endif
};

static const struct vm_operations_struct shmem_anon_vm_ops = {
	.fault		= shmem_fault,
	.map_pages	= filemap_map_pages,
#ifdef CONFIG_NUMA
	.set_policy     = shmem_set_policy,
	.get_policy     = shmem_get_policy,
#endif
};

int shmem_init_fs_context(struct fs_context *fc)
{
	struct shmem_options *ctx;

	ctx = kzalloc_obj(struct shmem_options);
	if (!ctx)
		return -ENOMEM;

	ctx->mode = 0777 | S_ISVTX;
	ctx->uid = current_fsuid();
	ctx->gid = current_fsgid();


	fc->fs_private = ctx;
	fc->ops = &shmem_fs_context_ops;
	return 0;
}

static struct file_system_type shmem_fs_type = {
	.owner		= THIS_MODULE,
	.name		= "tmpfs",
	.init_fs_context = shmem_init_fs_context,
	.kill_sb	= kill_anon_super,
	.fs_flags	= FS_USERNS_MOUNT | FS_ALLOW_IDMAP | FS_MGTIME,
};


void __init shmem_init(void)
{
	int error;

	shmem_init_inodecache();


	error = register_filesystem(&shmem_fs_type);
	if (error) {
		pr_err("Could not register tmpfs\n");
		goto out2;
	}

	shm_mnt = kern_mount(&shmem_fs_type);
	if (IS_ERR(shm_mnt)) {
		error = PTR_ERR(shm_mnt);
		pr_err("Could not kern_mount tmpfs\n");
		goto out1;
	}


	return;

out1:
	unregister_filesystem(&shmem_fs_type);
out2:
	shmem_destroy_inodecache();
	shm_mnt = ERR_PTR(error);
}



#else /* !CONFIG_SHMEM */

/*
 * tiny-shmem: simple shmemfs and tmpfs using ramfs code
 *
 * This is intended for small system where the benefits of the full
 * shmem code (swap-backed and resource-limited) are outweighed by
 * their complexity. On systems without swap this code should be
 * effectively equivalent, but much lighter weight.
 */

static struct file_system_type shmem_fs_type = {
	.name		= "tmpfs",
	.init_fs_context = ramfs_init_fs_context,
	.parameters	= ramfs_fs_parameters,
	.kill_sb	= ramfs_kill_sb,
	.fs_flags	= FS_USERNS_MOUNT,
};

void __init shmem_init(void)
{
	BUG_ON(register_filesystem(&shmem_fs_type) != 0);

	shm_mnt = kern_mount(&shmem_fs_type);
	BUG_ON(IS_ERR(shm_mnt));
}

int shmem_unuse(unsigned int type)
{
	return 0;
}

int shmem_lock(struct file *file, int lock, struct ucounts *ucounts)
{
	return 0;
}

void shmem_unlock_mapping(struct address_space *mapping)
{
}

#ifdef CONFIG_MMU
unsigned long shmem_get_unmapped_area(struct file *file,
				      unsigned long addr, unsigned long len,
				      unsigned long pgoff, unsigned long flags)
{
	return mm_get_unmapped_area(file, addr, len, pgoff, flags);
}
#endif

void shmem_truncate_range(struct inode *inode, loff_t lstart, uoff_t lend)
{
	truncate_inode_pages_range(inode->i_mapping, lstart, lend);
}
EXPORT_SYMBOL_GPL(shmem_truncate_range);

#define shmem_vm_ops				generic_file_vm_ops
#define shmem_anon_vm_ops			generic_file_vm_ops
#define shmem_file_operations			ramfs_file_operations

static inline int shmem_acct_size(unsigned long flags, loff_t size)
{
	return 0;
}

static inline void shmem_unacct_size(unsigned long flags, loff_t size)
{
}

static inline struct inode *shmem_get_inode(struct mnt_idmap *idmap,
				struct super_block *sb, struct inode *dir,
				umode_t mode, dev_t dev, vma_flags_t flags)
{
	struct inode *inode = ramfs_get_inode(sb, dir, mode, dev);
	return inode ? inode : ERR_PTR(-ENOSPC);
}

#endif /* CONFIG_SHMEM */

/* common code */

static struct file *__shmem_file_setup(struct vfsmount *mnt, const char *name,
				       loff_t size, vma_flags_t flags,
				       unsigned int i_flags)
{
	const unsigned long shmem_flags =
		vma_flags_test(&flags, VMA_NORESERVE_BIT) ? SHMEM_F_NORESERVE : 0;
	struct inode *inode;
	struct file *res;

	if (IS_ERR(mnt))
		return ERR_CAST(mnt);

	if (size < 0 || size > MAX_LFS_FILESIZE)
		return ERR_PTR(-EINVAL);

	if (is_idmapped_mnt(mnt))
		return ERR_PTR(-EINVAL);

	if (shmem_acct_size(shmem_flags, size))
		return ERR_PTR(-ENOMEM);

	inode = shmem_get_inode(&nop_mnt_idmap, mnt->mnt_sb, NULL,
				S_IFREG | S_IRWXUGO, 0, flags);
	if (IS_ERR(inode)) {
		shmem_unacct_size(shmem_flags, size);
		return ERR_CAST(inode);
	}
	inode->i_flags |= i_flags;
	inode->i_size = size;
	clear_nlink(inode);	/* It is unlinked */
	res = ERR_PTR(ramfs_nommu_expand_for_mapping(inode, size));
	if (!IS_ERR(res))
		res = alloc_file_pseudo(inode, mnt, name, O_RDWR,
				&shmem_file_operations);
	if (IS_ERR(res))
		iput(inode);
	return res;
}

/**
 * shmem_kernel_file_setup - get an unlinked file living in tmpfs which must be
 * 	kernel internal.  There will be NO LSM permission checks against the
 * 	underlying inode.  So users of this interface must do LSM checks at a
 *	higher layer.  The users are the big_key and shm implementations.  LSM
 *	checks are provided at the key or shm level rather than the inode.
 * @name: name for dentry (to be seen in /proc/<pid>/maps)
 * @size: size to be set for the file
 * @flags: VMA_NORESERVE_BIT suppresses pre-accounting of the entire object size
 */
struct file *shmem_kernel_file_setup(const char *name, loff_t size,
				     vma_flags_t flags)
{
	return __shmem_file_setup(shm_mnt, name, size, flags, S_PRIVATE);
}
EXPORT_SYMBOL_GPL(shmem_kernel_file_setup);

/**
 * shmem_file_setup - get an unlinked file living in tmpfs
 * @name: name for dentry (to be seen in /proc/<pid>/maps)
 * @size: size to be set for the file
 * @flags: VMA_NORESERVE_BIT suppresses pre-accounting of the entire object size
 */
struct file *shmem_file_setup(const char *name, loff_t size, vma_flags_t flags)
{
	return __shmem_file_setup(shm_mnt, name, size, flags, 0);
}
EXPORT_SYMBOL_GPL(shmem_file_setup);

/**
 * shmem_file_setup_with_mnt - get an unlinked file living in tmpfs
 * @mnt: the tmpfs mount where the file will be created
 * @name: name for dentry (to be seen in /proc/<pid>/maps)
 * @size: size to be set for the file
 * @flags: VMA_NORESERVE_BIT suppresses pre-accounting of the entire object size
 */
struct file *shmem_file_setup_with_mnt(struct vfsmount *mnt, const char *name,
				       loff_t size, vma_flags_t flags)
{
	return __shmem_file_setup(mnt, name, size, flags, 0);
}
EXPORT_SYMBOL_GPL(shmem_file_setup_with_mnt);

static struct file *__shmem_zero_setup(unsigned long start, unsigned long end,
		vma_flags_t flags)
{
	loff_t size = end - start;

	/*
	 * Cloning a new file under mmap_lock leads to a lock ordering conflict
	 * between XFS directory reading and selinux: since this file is only
	 * accessible to the user through its mapping, use S_PRIVATE flag to
	 * bypass file security, in the same way as shmem_kernel_file_setup().
	 */
	return shmem_kernel_file_setup("dev/zero", size, flags);
}

/**
 * shmem_zero_setup - setup a shared anonymous mapping
 * @vma: the vma to be mmapped is prepared by do_mmap
 * Returns: 0 on success, or error
 */
int shmem_zero_setup(struct vm_area_struct *vma)
{
	struct file *file = __shmem_zero_setup(vma->vm_start, vma->vm_end, vma->flags);

	if (IS_ERR(file))
		return PTR_ERR(file);

	if (vma->vm_file)
		fput(vma->vm_file);
	vma->vm_file = file;
	vma->vm_ops = &shmem_anon_vm_ops;

	return 0;
}

/**
 * shmem_zero_setup_desc - same as shmem_zero_setup, but determined by VMA
 * descriptor for convenience.
 * @desc: Describes VMA
 * Returns: 0 on success, or error
 */
int shmem_zero_setup_desc(struct vm_area_desc *desc)
{
	struct file *file = __shmem_zero_setup(desc->start, desc->end, desc->vma_flags);

	if (IS_ERR(file))
		return PTR_ERR(file);

	desc->vm_file = file;
	desc->vm_ops = &shmem_anon_vm_ops;

	return 0;
}

/**
 * shmem_read_folio_gfp - read into page cache, using specified page allocation flags.
 * @mapping:	the folio's address_space
 * @index:	the folio index
 * @gfp:	the page allocator flags to use if allocating
 *
 * This behaves as a tmpfs "read_cache_page_gfp(mapping, index, gfp)",
 * with any new page allocations done using the specified allocation flags.
 * But read_cache_page_gfp() uses the ->read_folio() method: which does not
 * suit tmpfs, since it may have pages in swapcache, and needs to find those
 * for itself; although drivers/gpu/drm i915 and ttm rely upon this support.
 *
 * i915_gem_object_get_pages_gtt() mixes __GFP_NORETRY | __GFP_NOWARN in
 * with the mapping_gfp_mask(), to avoid OOMing the machine unnecessarily.
 */
struct folio *shmem_read_folio_gfp(struct address_space *mapping,
		pgoff_t index, gfp_t gfp)
{
#ifdef CONFIG_SHMEM
	struct inode *inode = mapping->host;
	struct folio *folio;
	int error;

	error = shmem_get_folio_gfp(inode, index, i_size_read(inode),
				    &folio, SGP_CACHE, gfp, NULL, NULL);
	if (error)
		return ERR_PTR(error);

	folio_unlock(folio);
	return folio;
#else
	/*
	 * The tiny !SHMEM case uses ramfs without swap
	 */
	return mapping_read_folio_gfp(mapping, index, gfp);
#endif
}
EXPORT_SYMBOL_GPL(shmem_read_folio_gfp);

struct page *shmem_read_mapping_page_gfp(struct address_space *mapping,
					 pgoff_t index, gfp_t gfp)
{
	struct folio *folio = shmem_read_folio_gfp(mapping, index, gfp);
	struct page *page;

	if (IS_ERR(folio))
		return &folio->page;

	page = folio_file_page(folio, index);
	if (PageHWPoison(page)) {
		folio_put(folio);
		return ERR_PTR(-EIO);
	}

	return page;
}
EXPORT_SYMBOL_GPL(shmem_read_mapping_page_gfp);
