/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Definitions for diskquota-operations. When diskquota is configured these
 * macros expand to the right source-code.
 *
 * Author:  Marco van Wieringen <mvw@planets.elm.net>
 */
#ifndef _LINUX_QUOTAOPS_
#define _LINUX_QUOTAOPS_

#include <linux/fs.h>

#define DQUOT_SPACE_WARN	0x1
#define DQUOT_SPACE_RESERVE	0x2
#define DQUOT_SPACE_NOFAIL	0x4

static inline struct quota_info *sb_dqopt(struct super_block *sb)
{
	return &sb->s_dquot;
}

/* i_rwsem must being held */
static inline bool is_quota_modification(struct mnt_idmap *idmap,
					 struct inode *inode, struct iattr *ia)
{
	return ((ia->ia_valid & ATTR_SIZE) ||
		i_uid_needs_update(idmap, ia, inode) ||
		i_gid_needs_update(idmap, ia, inode));
}


static inline int sb_has_quota_usage_enabled(struct super_block *sb, int type)
{
	return 0;
}

static inline int sb_has_quota_limits_enabled(struct super_block *sb, int type)
{
	return 0;
}

static inline int sb_has_quota_suspended(struct super_block *sb, int type)
{
	return 0;
}

static inline int sb_any_quota_suspended(struct super_block *sb)
{
	return 0;
}

/* Does kernel know about any quota information for given sb + type? */
static inline int sb_has_quota_loaded(struct super_block *sb, int type)
{
	return 0;
}

static inline int sb_any_quota_loaded(struct super_block *sb)
{
	return 0;
}

static inline int sb_has_quota_active(struct super_block *sb, int type)
{
	return 0;
}

static inline int dquot_initialize(struct inode *inode)
{
	return 0;
}

static inline bool dquot_initialize_needed(struct inode *inode)
{
	return false;
}

static inline void dquot_drop(struct inode *inode)
{
}

static inline int dquot_alloc_inode(struct inode *inode)
{
	return 0;
}

static inline void dquot_free_inode(struct inode *inode)
{
}

static inline int dquot_transfer(struct mnt_idmap *idmap,
				 struct inode *inode, struct iattr *iattr)
{
	return 0;
}

static inline int __dquot_alloc_space(struct inode *inode, qsize_t number,
		int flags)
{
	if (!(flags & DQUOT_SPACE_RESERVE))
		inode_add_bytes(inode, number);
	return 0;
}

static inline void __dquot_free_space(struct inode *inode, qsize_t number,
		int flags)
{
	if (!(flags & DQUOT_SPACE_RESERVE))
		inode_sub_bytes(inode, number);
}

static inline void dquot_claim_space_nodirty(struct inode *inode, qsize_t number)
{
	inode_add_bytes(inode, number);
}

static inline int dquot_reclaim_space_nodirty(struct inode *inode,
					      qsize_t number)
{
	inode_sub_bytes(inode, number);
	return 0;
}

static inline int dquot_disable(struct super_block *sb, int type,
		unsigned int flags)
{
	return 0;
}

static inline int dquot_suspend(struct super_block *sb, int type)
{
	return 0;
}

static inline int dquot_resume(struct super_block *sb, int type)
{
	return 0;
}

#define dquot_file_open		generic_file_open

static inline int dquot_writeback_dquots(struct super_block *sb, int type)
{
	return 0;
}


static inline int dquot_alloc_space_nodirty(struct inode *inode, qsize_t nr)
{
	return __dquot_alloc_space(inode, nr, DQUOT_SPACE_WARN);
}

static inline void dquot_alloc_space_nofail(struct inode *inode, qsize_t nr)
{
	__dquot_alloc_space(inode, nr, DQUOT_SPACE_WARN|DQUOT_SPACE_NOFAIL);
	mark_inode_dirty_sync(inode);
}

static inline int dquot_alloc_space(struct inode *inode, qsize_t nr)
{
	int ret;

	ret = dquot_alloc_space_nodirty(inode, nr);
	if (!ret) {
		/*
		 * Mark inode fully dirty. Since we are allocating blocks, inode
		 * would become fully dirty soon anyway and it reportedly
		 * reduces lock contention.
		 */
		mark_inode_dirty(inode);
	}
	return ret;
}

static inline int dquot_alloc_block_nodirty(struct inode *inode, qsize_t nr)
{
	return dquot_alloc_space_nodirty(inode, nr << inode->i_blkbits);
}

static inline void dquot_alloc_block_nofail(struct inode *inode, qsize_t nr)
{
	dquot_alloc_space_nofail(inode, nr << inode->i_blkbits);
}

static inline int dquot_alloc_block(struct inode *inode, qsize_t nr)
{
	return dquot_alloc_space(inode, nr << inode->i_blkbits);
}

static inline int dquot_prealloc_block_nodirty(struct inode *inode, qsize_t nr)
{
	return __dquot_alloc_space(inode, nr << inode->i_blkbits, 0);
}

static inline int dquot_prealloc_block(struct inode *inode, qsize_t nr)
{
	int ret;

	ret = dquot_prealloc_block_nodirty(inode, nr);
	if (!ret)
		mark_inode_dirty_sync(inode);
	return ret;
}

static inline int dquot_reserve_block(struct inode *inode, qsize_t nr)
{
	return __dquot_alloc_space(inode, nr << inode->i_blkbits,
				DQUOT_SPACE_WARN|DQUOT_SPACE_RESERVE);
}

static inline void dquot_claim_block(struct inode *inode, qsize_t nr)
{
	dquot_claim_space_nodirty(inode, nr << inode->i_blkbits);
	mark_inode_dirty_sync(inode);
}

static inline void dquot_reclaim_block(struct inode *inode, qsize_t nr)
{
	dquot_reclaim_space_nodirty(inode, nr << inode->i_blkbits);
	mark_inode_dirty_sync(inode);
}

static inline void dquot_free_space_nodirty(struct inode *inode, qsize_t nr)
{
	__dquot_free_space(inode, nr, 0);
}

static inline void dquot_free_space(struct inode *inode, qsize_t nr)
{
	dquot_free_space_nodirty(inode, nr);
	mark_inode_dirty_sync(inode);
}

static inline void dquot_free_block_nodirty(struct inode *inode, qsize_t nr)
{
	dquot_free_space_nodirty(inode, nr << inode->i_blkbits);
}

static inline void dquot_free_block(struct inode *inode, qsize_t nr)
{
	dquot_free_space(inode, nr << inode->i_blkbits);
}

static inline void dquot_release_reservation_block(struct inode *inode,
		qsize_t nr)
{
	__dquot_free_space(inode, nr << inode->i_blkbits, DQUOT_SPACE_RESERVE);
}

unsigned int qtype_enforce_flag(int type);

#endif /* _LINUX_QUOTAOPS_ */
