/* SPDX-License-Identifier: GPL-2.0-or-later */
/* audit.h -- Auditing support
 *
 * Copyright 2003-2004 Red Hat Inc., Durham, North Carolina.
 * All Rights Reserved.
 *
 * Written by Rickard E. (Rik) Faith <faith@redhat.com>
 */
#ifndef _LINUX_AUDIT_H_
#define _LINUX_AUDIT_H_

#include <linux/sched.h>
#include <linux/ptrace.h>
#include <linux/audit_arch.h>
#include <uapi/linux/audit.h>
#include <uapi/linux/fanotify.h>

#define AUDIT_STATUS_ALL (AUDIT_STATUS_ENABLED | \
			  AUDIT_STATUS_FAILURE | \
			  AUDIT_STATUS_PID | \
			  AUDIT_STATUS_RATE_LIMIT | \
			  AUDIT_STATUS_BACKLOG_LIMIT | \
			  AUDIT_STATUS_BACKLOG_WAIT_TIME | \
			  AUDIT_STATUS_LOST | \
			  AUDIT_STATUS_BACKLOG_WAIT_TIME_ACTUAL)

#define AUDIT_INO_UNSET ((u64)-1)
#define AUDIT_DEV_UNSET ((dev_t)-1)

struct audit_sig_info {
	uid_t		uid;
	pid_t		pid;
	char		ctx[];
};

struct audit_buffer;
struct audit_context;
struct inode;
struct path;
struct linux_binprm;
struct mq_attr;
struct mqstat;
struct audit_watch;
struct audit_tree;
struct kern_ipc_perm;
struct lsm_id;
struct lsm_prop;

struct audit_krule {
	u32			pflags;
	u32			flags;
	u32			listnr;
	u32			action;
	u32			mask[AUDIT_BITMASK_SIZE];
	u32			buflen; /* for data alloc on list rules */
	u32			field_count;
	char			*filterkey; /* ties events to rules */
	struct audit_field	*fields;
	struct audit_field	*arch_f; /* quick access to arch field */
	struct audit_field	*inode_f; /* quick access to an inode field */
	struct audit_watch	*watch;	/* associated watch */
	struct audit_tree	*tree;	/* associated watched tree */
	struct audit_fsnotify_mark	*exe;
	struct list_head	rlist;	/* entry in audit_{watch,tree}.rules list */
	struct list_head	list;	/* for AUDIT_LIST* purposes only */
	u64			prio;
};

/* Flag to indicate legacy AUDIT_LOGINUID unset usage */
#define AUDIT_LOGINUID_LEGACY		0x1

struct audit_field {
	u32				type;
	union {
		u32			val;
		kuid_t			uid;
		kgid_t			gid;
		struct {
			char		*lsm_str;
			void		*lsm_rule;
		};
	};
	u32				op;
};

enum audit_ntp_type {
	AUDIT_NTP_OFFSET,
	AUDIT_NTP_FREQ,
	AUDIT_NTP_STATUS,
	AUDIT_NTP_TAI,
	AUDIT_NTP_TICK,
	AUDIT_NTP_ADJUST,

	AUDIT_NTP_NVALS /* count */
};

struct audit_ntp_data {};

extern int __init audit_register_class(int class, unsigned *list);
extern int audit_classify_syscall(int abi, unsigned syscall);
extern int audit_classify_arch(int arch);

/* audit_names->type values */
#define	AUDIT_TYPE_UNKNOWN	0	/* we don't know yet */
#define	AUDIT_TYPE_NORMAL	1	/* a "normal" audit record */
#define	AUDIT_TYPE_PARENT	2	/* a parent audit record */
#define	AUDIT_TYPE_CHILD_DELETE 3	/* a child being deleted */
#define	AUDIT_TYPE_CHILD_CREATE 4	/* a child being created */

/* maximized args number that audit_socketcall can process */
#define AUDITSC_ARGS		6

/* bit values for ->signal->audit_tty */
#define AUDIT_TTY_ENABLE	BIT(0)
#define AUDIT_TTY_LOG_PASSWD	BIT(1)

/* bit values for audit_cfg_lsm */
#define AUDIT_CFG_LSM_SECCTX_SUBJECT	BIT(0)
#define AUDIT_CFG_LSM_SECCTX_OBJECT	BIT(1)

struct filename;

#define AUDIT_OFF	0
#define AUDIT_ON	1
#define AUDIT_LOCKED	2
static inline __printf(4, 5)
void audit_log(struct audit_context *ctx, gfp_t gfp_mask, int type,
	       const char *fmt, ...)
{ }
static inline struct audit_buffer *audit_log_start(struct audit_context *ctx,
						   gfp_t gfp_mask, int type)
{
	return NULL;
}
static inline __printf(2, 3)
void audit_log_format(struct audit_buffer *ab, const char *fmt, ...)
{ }
static inline void audit_log_end(struct audit_buffer *ab)
{ }
static inline void audit_log_n_hex(struct audit_buffer *ab,
				   const unsigned char *buf, size_t len)
{ }
static inline void audit_log_n_string(struct audit_buffer *ab,
				      const char *buf, size_t n)
{ }
static inline void  audit_log_n_untrustedstring(struct audit_buffer *ab,
						const char *string, size_t n)
{ }
static inline void audit_log_untrustedstring(struct audit_buffer *ab,
					     const char *string)
{ }
static inline void audit_log_d_path(struct audit_buffer *ab,
				    const char *prefix,
				    const struct path *path)
{ }
static inline void audit_log_key(struct audit_buffer *ab, char *key)
{ }
static inline void audit_log_path_denied(int type, const char *operation)
{ }
static inline int audit_log_subj_ctx(struct audit_buffer *ab,
				     struct lsm_prop *prop)
{
	return 0;
}
static inline int audit_log_obj_ctx(struct audit_buffer *ab,
				    struct lsm_prop *prop)
{
	return 0;
}
static inline int audit_log_task_context(struct audit_buffer *ab)
{
	return 0;
}
static inline void audit_log_task_info(struct audit_buffer *ab)
{ }

static inline kuid_t audit_get_loginuid(struct task_struct *tsk)
{
	return INVALID_UID;
}

static inline unsigned int audit_get_sessionid(struct task_struct *tsk)
{
	return AUDIT_SID_UNSET;
}

#define audit_enabled AUDIT_OFF

static inline int audit_signal_info(int sig, struct task_struct *t)
{
	return 0;
}

static inline void audit_cfg_lsm(const struct lsm_id *lsmid, int flags)
{ }

#ifdef CONFIG_AUDIT_COMPAT_GENERIC
#define audit_is_compat(arch)  (!((arch) & __AUDIT_ARCH_64BIT))
#else
#define audit_is_compat(arch)  false
#endif

#define AUDIT_INODE_PARENT	1	/* dentry represents the parent */
#define AUDIT_INODE_HIDDEN	2	/* audit record should be hidden */
#define AUDIT_INODE_NOEVAL	4	/* audit record incomplete */

#include <asm/syscall.h> /* for syscall_get_arch() */

static inline int audit_alloc(struct task_struct *task)
{
	return 0;
}
static inline void audit_free(struct task_struct *task)
{ }
static inline void audit_uring_entry(u8 op)
{ }
static inline void audit_uring_exit(int success, long code)
{ }
static inline void audit_syscall_entry(int major, unsigned long a0,
				       unsigned long a1, unsigned long a2,
				       unsigned long a3)
{ }
static inline void audit_syscall_exit(void *pt_regs)
{ }
static inline bool audit_dummy_context(void)
{
	return true;
}
static inline void audit_set_context(struct task_struct *task, struct audit_context *ctx)
{ }
static inline struct audit_context *audit_context(void)
{
	return NULL;
}
static inline void audit_getname(struct filename *name)
{ }
static inline void audit_inode(struct filename *name,
				const struct dentry *dentry,
				unsigned int aflags)
{ }
static inline void audit_file(struct file *file)
{
}
static inline void audit_inode_parent_hidden(struct filename *name,
				const struct dentry *dentry)
{ }
static inline void audit_inode_child(struct inode *parent,
				     const struct dentry *dentry,
				     const unsigned char type)
{ }
static inline void audit_core_dumps(long signr)
{ }
static inline void audit_seccomp(unsigned long syscall, long signr, int code)
{ }
static inline void audit_seccomp_actions_logged(const char *names,
						const char *old_names, int res)
{ }
static inline void audit_ipc_obj(struct kern_ipc_perm *ipcp)
{ }
static inline void audit_ipc_set_perm(unsigned long qbytes, uid_t uid,
					gid_t gid, umode_t mode)
{ }
static inline void audit_bprm(struct linux_binprm *bprm)
{ }
static inline int audit_socketcall(int nargs, unsigned long *args)
{
	return 0;
}

static inline int audit_socketcall_compat(int nargs, u32 *args)
{
	return 0;
}

static inline void audit_fd_pair(int fd1, int fd2)
{ }
static inline int audit_sockaddr(int len, void *addr)
{
	return 0;
}
static inline void audit_mq_open(int oflag, umode_t mode, struct mq_attr *attr)
{ }
static inline void audit_mq_sendrecv(mqd_t mqdes, size_t msg_len,
				     unsigned int msg_prio,
				     const struct timespec64 *abs_timeout)
{ }
static inline void audit_mq_notify(mqd_t mqdes,
				   const struct sigevent *notification)
{ }
static inline void audit_mq_getsetattr(mqd_t mqdes, struct mq_attr *mqstat)
{ }
static inline int audit_log_bprm_fcaps(struct linux_binprm *bprm,
				       const struct cred *new,
				       const struct cred *old)
{
	return 0;
}
static inline void audit_log_capset(const struct cred *new,
				    const struct cred *old)
{ }
static inline void audit_mmap_fd(int fd, int flags)
{ }

static inline void audit_openat2_how(struct open_how *how)
{ }

static inline void audit_log_kern_module(const char *name)
{ }

static inline void audit_fanotify(u32 response, struct fanotify_response_info_audit_rule *friar)
{ }

static inline void audit_tk_injoffset(struct timespec64 offset)
{ }

static inline void audit_ntp_init(struct audit_ntp_data *ad)
{ }

static inline void audit_ntp_set_old(struct audit_ntp_data *ad,
				     enum audit_ntp_type type, long long val)
{ }

static inline void audit_ntp_set_new(struct audit_ntp_data *ad,
				     enum audit_ntp_type type, long long val)
{ }

static inline void audit_ntp_log(const struct audit_ntp_data *ad)
{ }

static inline void audit_ptrace(struct task_struct *t)
{ }

#define audit_n_rules 0
#define audit_signals 0

static inline bool audit_loginuid_set(struct task_struct *tsk)
{
	return uid_valid(audit_get_loginuid(tsk));
}

#endif
