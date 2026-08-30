/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Filesystem access notification for Linux
 *
 *  Copyright (C) 2008 Red Hat, Inc., Eric Paris <eparis@redhat.com>
 */

#ifndef __LINUX_FSNOTIFY_BACKEND_H
#define __LINUX_FSNOTIFY_BACKEND_H

#ifdef __KERNEL__

#include <linux/idr.h> /* inotify uses this */
#include <linux/fs.h> /* struct inode */
#include <linux/list.h>
#include <linux/path.h> /* struct path */
#include <linux/spinlock.h>
#include <linux/types.h>
#include <linux/atomic.h>
#include <linux/user_namespace.h>
#include <linux/refcount.h>
#include <linux/mempool.h>
#include <linux/sched/mm.h>

/*
 * IN_* from inotfy.h lines up EXACTLY with FS_*, this is so we can easily
 * convert between them.  dnotify only needs conversion at watch creation
 * so no perf loss there.  fanotify isn't defined yet, so it can use the
 * wholes if it needs more events.
 */
#define FS_ACCESS		0x00000001	/* File was accessed */
#define FS_MODIFY		0x00000002	/* File was modified */
#define FS_ATTRIB		0x00000004	/* Metadata changed */
#define FS_CLOSE_WRITE		0x00000008	/* Writable file was closed */
#define FS_CLOSE_NOWRITE	0x00000010	/* Unwritable file closed */
#define FS_OPEN			0x00000020	/* File was opened */
#define FS_MOVED_FROM		0x00000040	/* File was moved from X */
#define FS_MOVED_TO		0x00000080	/* File was moved to Y */
#define FS_CREATE		0x00000100	/* Subfile was created */
#define FS_DELETE		0x00000200	/* Subfile was deleted */
#define FS_DELETE_SELF		0x00000400	/* Self was deleted */
#define FS_MOVE_SELF		0x00000800	/* Self was moved */
#define FS_OPEN_EXEC		0x00001000	/* File was opened for exec */

#define FS_UNMOUNT		0x00002000	/* inode on umount fs */
#define FS_Q_OVERFLOW		0x00004000	/* Event queued overflowed */
#define FS_ERROR		0x00008000	/* Filesystem Error (fanotify) */

/*
 * FS_IN_IGNORED overloads FS_ERROR.  It is only used internally by inotify
 * which does not support FS_ERROR.
 */
#define FS_IN_IGNORED		0x00008000	/* last inotify event here */

#define FS_OPEN_PERM		0x00010000	/* open event in an permission hook */
#define FS_ACCESS_PERM		0x00020000	/* access event in a permissions hook */
#define FS_OPEN_EXEC_PERM	0x00040000	/* open/exec event in a permission hook */
/* #define FS_DIR_MODIFY	0x00080000 */	/* Deprecated (reserved) */

#define FS_PRE_ACCESS		0x00100000	/* Pre-content access hook */

#define FS_MNT_ATTACH		0x01000000	/* Mount was attached */
#define FS_MNT_DETACH		0x02000000	/* Mount was detached */
#define FS_MNT_MOVE		(FS_MNT_ATTACH | FS_MNT_DETACH)

/*
 * Set on inode mark that cares about things that happen to its children.
 * Always set for dnotify and inotify.
 * Set on inode/sb/mount marks that care about parent/name info.
 */
#define FS_EVENT_ON_CHILD	0x08000000

#define FS_RENAME		0x10000000	/* File was renamed */
#define FS_DN_MULTISHOT		0x20000000	/* dnotify multishot */
#define FS_ISDIR		0x40000000	/* event occurred against dir */

#define FS_MOVE			(FS_MOVED_FROM | FS_MOVED_TO)

/*
 * Directory entry modification events - reported only to directory
 * where entry is modified and not to a watching parent.
 * The watching parent may get an FS_ATTRIB|FS_EVENT_ON_CHILD event
 * when a directory entry inside a child subdir changes.
 */
#define ALL_FSNOTIFY_DIRENT_EVENTS (FS_CREATE | FS_DELETE | FS_MOVE | FS_RENAME)

/* Mount namespace events */
#define FSNOTIFY_MNT_EVENTS (FS_MNT_ATTACH | FS_MNT_DETACH)

/* Content events can be used to inspect file content */
#define FSNOTIFY_CONTENT_PERM_EVENTS (FS_OPEN_PERM | FS_OPEN_EXEC_PERM | \
				      FS_ACCESS_PERM)
/* Pre-content events can be used to fill file content */
#define FSNOTIFY_PRE_CONTENT_EVENTS  (FS_PRE_ACCESS)

#define ALL_FSNOTIFY_PERM_EVENTS (FSNOTIFY_CONTENT_PERM_EVENTS | \
				  FSNOTIFY_PRE_CONTENT_EVENTS)

/*
 * This is a list of all events that may get sent to a parent that is watching
 * with flag FS_EVENT_ON_CHILD based on fs event on a child of that directory.
 */
#define FS_EVENTS_POSS_ON_CHILD   (ALL_FSNOTIFY_PERM_EVENTS | \
				   FS_ACCESS | FS_MODIFY | FS_ATTRIB | \
				   FS_CLOSE_WRITE | FS_CLOSE_NOWRITE | \
				   FS_OPEN | FS_OPEN_EXEC)

/*
 * This is a list of all events that may get sent with the parent inode as the
 * @to_tell argument of fsnotify().
 * It may include events that can be sent to an inode/sb/mount mark, but cannot
 * be sent to a parent watching children.
 */
#define FS_EVENTS_POSS_TO_PARENT (FS_EVENTS_POSS_ON_CHILD)

/* Events that can be reported to backends */
#define ALL_FSNOTIFY_EVENTS (ALL_FSNOTIFY_DIRENT_EVENTS | \
			     FSNOTIFY_MNT_EVENTS | \
			     FS_EVENTS_POSS_ON_CHILD | \
			     FS_DELETE_SELF | FS_MOVE_SELF | \
			     FS_UNMOUNT | FS_Q_OVERFLOW | FS_IN_IGNORED | \
			     FS_ERROR)

/* Extra flags that may be reported with event or control handling of events */
#define ALL_FSNOTIFY_FLAGS  (FS_ISDIR | FS_EVENT_ON_CHILD | FS_DN_MULTISHOT)

#define ALL_FSNOTIFY_BITS   (ALL_FSNOTIFY_EVENTS | ALL_FSNOTIFY_FLAGS)

struct fsnotify_group;
struct fsnotify_event;
struct fsnotify_mark;
struct fsnotify_event_private_data;
struct fsnotify_fname;
struct fsnotify_iter_info;

/*
 * Each group much define these ops.  The fsnotify infrastructure will call
 * these operations for each relevant group.
 *
 * handle_event - main call for a group to handle an fs event
 * @group:	group to notify
 * @mask:	event type and flags
 * @data:	object that event happened on
 * @data_type:	type of object for fanotify_data_XXX() accessors
 * @dir:	optional directory associated with event -
 *		if @file_name is not NULL, this is the directory that
 *		@file_name is relative to
 * @file_name:	optional file name associated with event
 * @cookie:	inotify rename cookie
 * @iter_info:	array of marks from this group that are interested in the event
 *
 * handle_inode_event - simple variant of handle_event() for groups that only
 *		have inode marks and don't have ignore mask
 * @mark:	mark to notify
 * @mask:	event type and flags
 * @inode:	inode that event happened on
 * @dir:	optional directory associated with event -
 *		if @file_name is not NULL, this is the directory that
 *		@file_name is relative to.
 *		Either @inode or @dir must be non-NULL.
 * @file_name:	optional file name associated with event
 * @cookie:	inotify rename cookie
 *
 * free_group_priv - called when a group refcnt hits 0 to clean up the private union
 * freeing_mark - called when a mark is being destroyed for some reason.  The group
 *		MUST be holding a reference on each mark and that reference must be
 *		dropped in this function.  inotify uses this function to send
 *		userspace messages that marks have been removed.
 */
struct fsnotify_ops {
	int (*handle_event)(struct fsnotify_group *group, u32 mask,
			    const void *data, int data_type, struct inode *dir,
			    const struct qstr *file_name, u32 cookie,
			    struct fsnotify_iter_info *iter_info);
	int (*handle_inode_event)(struct fsnotify_mark *mark, u32 mask,
			    struct inode *inode, struct inode *dir,
			    const struct qstr *file_name, u32 cookie);
	void (*free_group_priv)(struct fsnotify_group *group);
	void (*freeing_mark)(struct fsnotify_mark *mark, struct fsnotify_group *group);
	void (*free_event)(struct fsnotify_group *group, struct fsnotify_event *event);
	/* called on final put+free to free memory */
	void (*free_mark)(struct fsnotify_mark *mark);
};

/*
 * all of the information about the original object we want to now send to
 * a group.  If you want to carry more info from the accessing task to the
 * listener this structure is where you need to be adding fields.
 */
struct fsnotify_event {
	struct list_head list;
};

/*
 * fsnotify group priorities.
 * Events are sent in order from highest priority to lowest priority.
 */
enum fsnotify_group_prio {
	FSNOTIFY_PRIO_NORMAL = 0,	/* normal notifiers, no permissions */
	FSNOTIFY_PRIO_CONTENT,		/* fanotify permission events */
	FSNOTIFY_PRIO_PRE_CONTENT,	/* fanotify pre-content events */
	__FSNOTIFY_PRIO_NUM
};

/*
 * A group is a "thing" that wants to receive notification about filesystem
 * events.  The mask holds the subset of event types this group cares about.
 * refcnt on a group is up to the implementor and at any moment if it goes 0
 * everything will be cleaned up.
 */
struct fsnotify_group {
	const struct fsnotify_ops *ops;	/* how this group handles things */

	/*
	 * How the refcnt is used is up to each group.  When the refcnt hits 0
	 * fsnotify will clean up all of the resources associated with this group.
	 * As an example, the dnotify group will always have a refcnt=1 and that
	 * will never change.  Inotify, on the other hand, has a group per
	 * inotify_init() and the refcnt will hit 0 only when that fd has been
	 * closed.
	 */
	refcount_t refcnt;		/* things with interest in this group */

	/* needed to send notification to userspace */
	spinlock_t notification_lock;		/* protect the notification_list */
	struct list_head notification_list;	/* list of event_holder this group needs to send to userspace */
	wait_queue_head_t notification_waitq;	/* read() on the notification file blocks on this waitq */
	unsigned int q_len;			/* events on the queue */
	unsigned int max_events;		/* maximum events allowed on the list */
	enum fsnotify_group_prio priority;	/* priority for sending events */
	bool shutdown;		/* group is being shut down, don't queue more events */

#define FSNOTIFY_GROUP_USER	0x01 /* user allocated group */
#define FSNOTIFY_GROUP_DUPS	0x02 /* allow multiple marks per object */
	int flags;
	unsigned int owner_flags;	/* stored flags of mark_mutex owner */

	/* stores all fastpath marks assoc with this group so they can be cleaned on unregister */
	struct mutex mark_mutex;	/* protect marks_list */
	atomic_t user_waits;		/* Number of tasks waiting for user
					 * response */
	struct list_head marks_list;	/* all inode marks for this group */

	struct fasync_struct *fsn_fa;    /* async notification */

	struct fsnotify_event *overflow_event;	/* Event we queue when the
						 * notification list is too
						 * full */

	struct user_namespace *user_ns;	/* user ns where group was created */

	/* groups can define private fields here or use the void *private */
	union {
		void *private;
	};
};

/*
 * These helpers are used to prevent deadlock when reclaiming inodes with
 * evictable marks of the same group that is allocating a new mark.
 */
static inline void fsnotify_group_lock(struct fsnotify_group *group)
{
	mutex_lock(&group->mark_mutex);
	group->owner_flags = memalloc_nofs_save();
}

static inline void fsnotify_group_unlock(struct fsnotify_group *group)
{
	memalloc_nofs_restore(group->owner_flags);
	mutex_unlock(&group->mark_mutex);
}

static inline void fsnotify_group_assert_locked(struct fsnotify_group *group)
{
	WARN_ON_ONCE(!mutex_is_locked(&group->mark_mutex));
	WARN_ON_ONCE(!(current->flags & PF_MEMALLOC_NOFS));
}

/* When calling fsnotify tell it if the data is a path or inode */
enum fsnotify_data_type {
	FSNOTIFY_EVENT_NONE,
	FSNOTIFY_EVENT_FILE_RANGE,
	FSNOTIFY_EVENT_PATH,
	FSNOTIFY_EVENT_INODE,
	FSNOTIFY_EVENT_DENTRY,
	FSNOTIFY_EVENT_MNT,
	FSNOTIFY_EVENT_ERROR,
};

struct fs_error_report {
	int error;
	struct inode *inode;
	struct super_block *sb;
};

struct file_range {
	const struct path *path;
	loff_t pos;
	size_t count;
};

static inline const struct path *file_range_path(const struct file_range *range)
{
	return range->path;
}

struct fsnotify_mnt {
	const struct mnt_namespace *ns;
	u64 mnt_id;
};

static inline struct inode *fsnotify_data_inode(const void *data, int data_type)
{
	switch (data_type) {
	case FSNOTIFY_EVENT_INODE:
		return (struct inode *)data;
	case FSNOTIFY_EVENT_DENTRY:
		return d_inode(data);
	case FSNOTIFY_EVENT_PATH:
		return d_inode(((const struct path *)data)->dentry);
	case FSNOTIFY_EVENT_FILE_RANGE:
		return d_inode(file_range_path(data)->dentry);
	case FSNOTIFY_EVENT_ERROR:
		return ((struct fs_error_report *)data)->inode;
	default:
		return NULL;
	}
}

static inline struct dentry *fsnotify_data_dentry(const void *data, int data_type)
{
	switch (data_type) {
	case FSNOTIFY_EVENT_DENTRY:
		/* Non const is needed for dget() */
		return (struct dentry *)data;
	case FSNOTIFY_EVENT_PATH:
		return ((const struct path *)data)->dentry;
	case FSNOTIFY_EVENT_FILE_RANGE:
		return file_range_path(data)->dentry;
	default:
		return NULL;
	}
}

static inline const struct path *fsnotify_data_path(const void *data,
						    int data_type)
{
	switch (data_type) {
	case FSNOTIFY_EVENT_PATH:
		return data;
	case FSNOTIFY_EVENT_FILE_RANGE:
		return file_range_path(data);
	default:
		return NULL;
	}
}

static inline struct super_block *fsnotify_data_sb(const void *data,
						   int data_type)
{
	switch (data_type) {
	case FSNOTIFY_EVENT_INODE:
		return ((struct inode *)data)->i_sb;
	case FSNOTIFY_EVENT_DENTRY:
		return ((struct dentry *)data)->d_sb;
	case FSNOTIFY_EVENT_PATH:
		return ((const struct path *)data)->dentry->d_sb;
	case FSNOTIFY_EVENT_FILE_RANGE:
		return file_range_path(data)->dentry->d_sb;
	case FSNOTIFY_EVENT_ERROR:
		return ((struct fs_error_report *) data)->sb;
	default:
		return NULL;
	}
}

static inline const struct fsnotify_mnt *fsnotify_data_mnt(const void *data,
							   int data_type)
{
	switch (data_type) {
	case FSNOTIFY_EVENT_MNT:
		return data;
	default:
		return NULL;
	}
}

static inline u64 fsnotify_data_mnt_id(const void *data, int data_type)
{
	const struct fsnotify_mnt *mnt_data = fsnotify_data_mnt(data, data_type);

	return mnt_data ? mnt_data->mnt_id : 0;
}

static inline struct fs_error_report *fsnotify_data_error_report(
							const void *data,
							int data_type)
{
	switch (data_type) {
	case FSNOTIFY_EVENT_ERROR:
		return (struct fs_error_report *) data;
	default:
		return NULL;
	}
}

static inline const struct file_range *fsnotify_data_file_range(
							const void *data,
							int data_type)
{
	switch (data_type) {
	case FSNOTIFY_EVENT_FILE_RANGE:
		return (struct file_range *)data;
	default:
		return NULL;
	}
}

/*
 * Index to merged marks iterator array that correlates to a type of watch.
 * The type of watched object can be deduced from the iterator type, but not
 * the other way around, because an event can match different watched objects
 * of the same object type.
 * For example, both parent and child are watching an object of type inode.
 */
enum fsnotify_iter_type {
	FSNOTIFY_ITER_TYPE_INODE,
	FSNOTIFY_ITER_TYPE_VFSMOUNT,
	FSNOTIFY_ITER_TYPE_SB,
	FSNOTIFY_ITER_TYPE_PARENT,
	FSNOTIFY_ITER_TYPE_INODE2,
	FSNOTIFY_ITER_TYPE_MNTNS,
	FSNOTIFY_ITER_TYPE_COUNT
};

/* The type of object that a mark is attached to */
enum fsnotify_obj_type {
	FSNOTIFY_OBJ_TYPE_ANY = -1,
	FSNOTIFY_OBJ_TYPE_INODE,
	FSNOTIFY_OBJ_TYPE_VFSMOUNT,
	FSNOTIFY_OBJ_TYPE_SB,
	FSNOTIFY_OBJ_TYPE_MNTNS,
	FSNOTIFY_OBJ_TYPE_COUNT,
	FSNOTIFY_OBJ_TYPE_DETACHED = FSNOTIFY_OBJ_TYPE_COUNT
};

static inline bool fsnotify_valid_obj_type(unsigned int obj_type)
{
	return (obj_type < FSNOTIFY_OBJ_TYPE_COUNT);
}

struct fsnotify_iter_info {
	struct fsnotify_mark *marks[FSNOTIFY_ITER_TYPE_COUNT];
	struct fsnotify_group *current_group;
	unsigned int report_mask;
	int srcu_idx;
};

static inline bool fsnotify_iter_should_report_type(
		struct fsnotify_iter_info *iter_info, int iter_type)
{
	return (iter_info->report_mask & (1U << iter_type));
}

static inline void fsnotify_iter_set_report_type(
		struct fsnotify_iter_info *iter_info, int iter_type)
{
	iter_info->report_mask |= (1U << iter_type);
}

static inline struct fsnotify_mark *fsnotify_iter_mark(
		struct fsnotify_iter_info *iter_info, int iter_type)
{
	if (fsnotify_iter_should_report_type(iter_info, iter_type))
		return iter_info->marks[iter_type];
	return NULL;
}

static inline int fsnotify_iter_step(struct fsnotify_iter_info *iter, int type,
				     struct fsnotify_mark **markp)
{
	while (type < FSNOTIFY_ITER_TYPE_COUNT) {
		*markp = fsnotify_iter_mark(iter, type);
		if (*markp)
			break;
		type++;
	}
	return type;
}

#define FSNOTIFY_ITER_FUNCS(name, NAME) \
static inline struct fsnotify_mark *fsnotify_iter_##name##_mark( \
		struct fsnotify_iter_info *iter_info) \
{ \
	return fsnotify_iter_mark(iter_info, FSNOTIFY_ITER_TYPE_##NAME); \
}

FSNOTIFY_ITER_FUNCS(inode, INODE)
FSNOTIFY_ITER_FUNCS(parent, PARENT)
FSNOTIFY_ITER_FUNCS(vfsmount, VFSMOUNT)
FSNOTIFY_ITER_FUNCS(sb, SB)

#define fsnotify_foreach_iter_type(type) \
	for (type = 0; type < FSNOTIFY_ITER_TYPE_COUNT; type++)
#define fsnotify_foreach_iter_mark_type(iter, mark, type) \
	for (type = 0; \
	     type = fsnotify_iter_step(iter, type, &mark), \
	     type < FSNOTIFY_ITER_TYPE_COUNT; \
	     type++)

/*
 * Inode/vfsmount/sb point to this structure which tracks all marks attached to
 * the inode/vfsmount/sb. The reference to inode/vfsmount/sb is held by this
 * structure. We destroy this structure when there are no more marks attached
 * to it. The structure is protected by fsnotify_mark_srcu.
 */
struct fsnotify_mark_connector {
	spinlock_t lock;
	unsigned char type;	/* Type of object [lock] */
	unsigned char prio;	/* Highest priority group */
#define FSNOTIFY_CONN_FLAG_IS_WATCHED	0x01
#define FSNOTIFY_CONN_FLAG_HAS_IREF	0x02
	unsigned short flags;	/* flags [lock] */
	union {
		/* Object pointer [lock] */
		void *obj;
		/* Used listing heads to free after srcu period expires */
		struct fsnotify_mark_connector *destroy_next;
	};
	struct hlist_head list;	/* List of marks */
};

/*
 * Container for per-sb fsnotify state (sb marks and more).
 * Attached lazily on first marked object on the sb and freed when killing sb.
 */
struct fsnotify_sb_info {
	struct fsnotify_mark_connector __rcu *sb_marks;
	/* List of connectors for inode marks */
	struct list_head inode_conn_list;
	spinlock_t list_lock;	/* Lock protecting inode_conn_list */
	/*
	 * Number of inode/mount/sb objects that are being watched in this sb.
	 * Note that inodes objects are currently double-accounted.
	 *
	 * The value in watched_objects[prio] is the number of objects that are
	 * watched by groups of priority >= prio, so watched_objects[0] is the
	 * total number of watched objects in this sb.
	 */
	atomic_long_t watched_objects[__FSNOTIFY_PRIO_NUM];
};

static inline struct fsnotify_sb_info *fsnotify_sb_info(struct super_block *sb)
{
	return NULL;
}

static inline atomic_long_t *fsnotify_sb_watched_objects(struct super_block *sb)
{
	return &fsnotify_sb_info(sb)->watched_objects[0];
}

/*
 * A mark is simply an object attached to an in core inode which allows an
 * fsnotify listener to indicate they are either no longer interested in events
 * of a type matching mask or only interested in those events.
 *
 * These are flushed when an inode is evicted from core and may be flushed
 * when the inode is modified (as seen by fsnotify_access).  Some fsnotify
 * users (such as dnotify) will flush these when the open fd is closed and not
 * at inode eviction or modification.
 *
 * Text in brackets is showing the lock(s) protecting modifications of a
 * particular entry. obj_lock means either inode->i_lock or
 * mnt->mnt_root->d_lock depending on the mark type.
 */
struct fsnotify_mark {
	/* Mask this mark is for [mark->lock, group->mark_mutex] */
	__u32 mask;
	/* We hold one for presence in g_list. Also one ref for each 'thing'
	 * in kernel that found and may be using this mark. */
	refcount_t refcnt;
	/* Group this mark is for. Set on mark creation, stable until last ref
	 * is dropped */
	struct fsnotify_group *group;
	/* List of marks by group->marks_list. Also reused for queueing
	 * mark into destroy_list when it's waiting for the end of SRCU period
	 * before it can be freed. [group->mark_mutex] */
	struct list_head g_list;
	/* Protects inode / mnt pointers, flags, masks */
	spinlock_t lock;
	/* List of marks for inode / vfsmount [connector->lock, mark ref] */
	struct hlist_node obj_list;
	/* Head of list of marks for an object [mark ref] */
	struct fsnotify_mark_connector *connector;
	/* Events types and flags to ignore [mark->lock, group->mark_mutex] */
	__u32 ignore_mask;
	/* General fsnotify mark flags */
#define FSNOTIFY_MARK_FLAG_ALIVE		0x0001
#define FSNOTIFY_MARK_FLAG_ATTACHED		0x0002
	/* inotify mark flags */
#define FSNOTIFY_MARK_FLAG_EXCL_UNLINK		0x0010
#define FSNOTIFY_MARK_FLAG_IN_ONESHOT		0x0020
	/* fanotify mark flags */
#define FSNOTIFY_MARK_FLAG_IGNORED_SURV_MODIFY	0x0100
#define FSNOTIFY_MARK_FLAG_NO_IREF		0x0200
#define FSNOTIFY_MARK_FLAG_HAS_IGNORE_FLAGS	0x0400
#define FSNOTIFY_MARK_FLAG_HAS_FSID		0x0800
#define FSNOTIFY_MARK_FLAG_WEAK_FSID		0x1000
	unsigned int flags;		/* flags [mark->lock] */
};


static inline int fsnotify_pre_content(const struct path *path,
				       const loff_t *ppos, size_t count)
{
	return 0;
}

static inline int fsnotify(__u32 mask, const void *data, int data_type,
			   struct inode *dir, const struct qstr *name,
			   struct inode *inode, u32 cookie)
{
	return 0;
}

static inline int __fsnotify_parent(struct dentry *dentry, __u32 mask,
				  const void *data, int data_type)
{
	return 0;
}

static inline void __fsnotify_inode_delete(struct inode *inode)
{}

static inline void __fsnotify_vfsmount_delete(struct vfsmount *mnt)
{}

static inline void fsnotify_sb_delete(struct super_block *sb)
{}

static inline void __fsnotify_mntns_delete(struct mnt_namespace *mntns)
{}

static inline void fsnotify_sb_free(struct super_block *sb)
{}

static inline void fsnotify_update_flags(struct dentry *dentry)
{}

static inline u32 fsnotify_get_cookie(void)
{
	return 0;
}

static inline void fsnotify_unmount_inodes(struct super_block *sb)
{}

static inline void fsnotify_mnt(__u32 mask, struct mnt_namespace *ns, struct vfsmount *mnt)
{}


#endif	/* __KERNEL __ */

#endif	/* __LINUX_FSNOTIFY_BACKEND_H */
