// SPDX-License-Identifier: GPL-2.0
/*
 * linux/kernel/seccomp.c
 *
 * Copyright 2004-2005  Andrea Arcangeli <andrea@cpushare.com>
 *
 * Copyright (C) 2012 Google, Inc.
 * Will Drewry <wad@chromium.org>
 *
 * This defines a simple but solid secure-computing facility.
 *
 * Mode 1 uses a fixed list of allowed system calls.
 * Mode 2 allows user-defined system call filters in the form
 *        of Berkeley Packet Filters/Linux Socket Filters.
 */
#define pr_fmt(fmt) "seccomp: " fmt

#include <linux/refcount.h>
#include <linux/audit.h>
#include <linux/compat.h>
#include <linux/coredump.h>
#include <linux/kmemleak.h>
#include <linux/nospec.h>
#include <linux/prctl.h>
#include <linux/sched.h>
#include <linux/sched/task_stack.h>
#include <linux/seccomp.h>
#include <linux/slab.h>
#include <linux/syscalls.h>
#include <linux/sysctl.h>

#include <asm/syscall.h>

/* Not exposed in headers: strictly internal use only. */
#define SECCOMP_MODE_DEAD	(SECCOMP_MODE_FILTER + 1)


static inline bool seccomp_may_assign_mode(unsigned long seccomp_mode)
{
	assert_spin_locked(&current->sighand->siglock);

	if (current->seccomp.mode && current->seccomp.mode != seccomp_mode)
		return false;

	return true;
}

void __weak arch_seccomp_spec_mitigate(struct task_struct *task) { }

static inline void seccomp_assign_mode(struct task_struct *task,
				       unsigned long seccomp_mode,
				       unsigned long flags)
{
	assert_spin_locked(&task->sighand->siglock);

	task->seccomp.mode = seccomp_mode;
	/*
	 * Make sure SYSCALL_WORK_SECCOMP cannot be set before the mode (and
	 * filter) is set.
	 */
	smp_mb__before_atomic();
	/* Assume default seccomp processes want spec flaw mitigation. */
	if ((flags & SECCOMP_FILTER_FLAG_SPEC_ALLOW) == 0)
		arch_seccomp_spec_mitigate(task);
	set_task_syscall_work(task, SECCOMP);
}


/* For use with seccomp_actions_logged */
#define SECCOMP_LOG_KILL_PROCESS	(1 << 0)
#define SECCOMP_LOG_KILL_THREAD		(1 << 1)
#define SECCOMP_LOG_TRAP		(1 << 2)
#define SECCOMP_LOG_ERRNO		(1 << 3)
#define SECCOMP_LOG_TRACE		(1 << 4)
#define SECCOMP_LOG_LOG			(1 << 5)
#define SECCOMP_LOG_ALLOW		(1 << 6)
#define SECCOMP_LOG_USER_NOTIF		(1 << 7)

static u32 seccomp_actions_logged = SECCOMP_LOG_KILL_PROCESS |
				    SECCOMP_LOG_KILL_THREAD  |
				    SECCOMP_LOG_TRAP  |
				    SECCOMP_LOG_ERRNO |
				    SECCOMP_LOG_USER_NOTIF |
				    SECCOMP_LOG_TRACE |
				    SECCOMP_LOG_LOG;

static inline void seccomp_log(unsigned long syscall, long signr, u32 action,
			       bool requested)
{
	bool log = false;

	switch (action) {
	case SECCOMP_RET_ALLOW:
		break;
	case SECCOMP_RET_TRAP:
		log = requested && seccomp_actions_logged & SECCOMP_LOG_TRAP;
		break;
	case SECCOMP_RET_ERRNO:
		log = requested && seccomp_actions_logged & SECCOMP_LOG_ERRNO;
		break;
	case SECCOMP_RET_TRACE:
		log = requested && seccomp_actions_logged & SECCOMP_LOG_TRACE;
		break;
	case SECCOMP_RET_USER_NOTIF:
		log = requested && seccomp_actions_logged & SECCOMP_LOG_USER_NOTIF;
		break;
	case SECCOMP_RET_LOG:
		log = seccomp_actions_logged & SECCOMP_LOG_LOG;
		break;
	case SECCOMP_RET_KILL_THREAD:
		log = seccomp_actions_logged & SECCOMP_LOG_KILL_THREAD;
		break;
	case SECCOMP_RET_KILL_PROCESS:
	default:
		log = seccomp_actions_logged & SECCOMP_LOG_KILL_PROCESS;
	}

	/*
	 * Emit an audit message when the action is RET_KILL_*, RET_LOG, or the
	 * FILTER_FLAG_LOG bit was set. The admin has the ability to silence
	 * any action from being logged by removing the action name from the
	 * seccomp_actions_logged sysctl.
	 */
	if (!log)
		return;

	audit_seccomp(syscall, signr, action);
}

/*
 * Secure computing mode 1 allows only read/write/exit/sigreturn.
 * To be fully secure this must be combined with rlimit
 * to limit the stack allocations too.
 */
static const int mode1_syscalls[] = {
	__NR_seccomp_read, __NR_seccomp_write, __NR_seccomp_exit, __NR_seccomp_sigreturn,
#ifdef __NR_uretprobe
	__NR_uretprobe,
#endif
#ifdef __NR_uprobe
	__NR_uprobe,
#endif
	-1, /* negative terminated */
};

static void __secure_computing_strict(int this_syscall)
{
	const int *allowed_syscalls = mode1_syscalls;
#ifdef CONFIG_COMPAT
	if (in_compat_syscall())
		allowed_syscalls = get_compat_mode1_syscalls();
#endif
	do {
		if (*allowed_syscalls == this_syscall)
			return;
	} while (*++allowed_syscalls != -1);

#ifdef SECCOMP_DEBUG
	dump_stack();
#endif
	current->seccomp.mode = SECCOMP_MODE_DEAD;
	seccomp_log(this_syscall, SIGKILL, SECCOMP_RET_KILL_THREAD, true);
	do_exit(SIGKILL);
}

#ifndef CONFIG_HAVE_ARCH_SECCOMP_FILTER
void secure_computing_strict(int this_syscall)
{
	int mode = current->seccomp.mode;

	if (IS_ENABLED(CONFIG_CHECKPOINT_RESTORE) &&
	    unlikely(current->ptrace & PT_SUSPEND_SECCOMP))
		return;

	if (mode == SECCOMP_MODE_DISABLED)
		return;
	else if (mode == SECCOMP_MODE_STRICT)
		__secure_computing_strict(this_syscall);
	else
		BUG();
}
int __secure_computing(void)
{
	int this_syscall = syscall_get_nr(current, current_pt_regs());

	secure_computing_strict(this_syscall);
	return 0;
}
#else

static int __seccomp_filter(int this_syscall, const bool recheck_after_trace)
{
	BUG();

	return -1;
}

int __secure_computing(void)
{
	int mode = current->seccomp.mode;
	int this_syscall;

	if (IS_ENABLED(CONFIG_CHECKPOINT_RESTORE) &&
	    unlikely(current->ptrace & PT_SUSPEND_SECCOMP))
		return 0;

	this_syscall = syscall_get_nr(current, current_pt_regs());

	switch (mode) {
	case SECCOMP_MODE_STRICT:
		__secure_computing_strict(this_syscall);  /* may call do_exit */
		return 0;
	case SECCOMP_MODE_FILTER:
		return __seccomp_filter(this_syscall, false);
	/* Surviving SECCOMP_RET_KILL_* must be proactively impossible. */
	case SECCOMP_MODE_DEAD:
		WARN_ON_ONCE(1);
		do_exit(SIGKILL);
		return -1;
	default:
		BUG();
	}
}
#endif /* CONFIG_HAVE_ARCH_SECCOMP_FILTER */

long prctl_get_seccomp(void)
{
	return current->seccomp.mode;
}

/**
 * seccomp_set_mode_strict: internal function for setting strict seccomp
 *
 * Once current->seccomp.mode is non-zero, it may not be changed.
 *
 * Returns 0 on success or -EINVAL on failure.
 */
static long seccomp_set_mode_strict(void)
{
	const unsigned long seccomp_mode = SECCOMP_MODE_STRICT;
	long ret = -EINVAL;

	spin_lock_irq(&current->sighand->siglock);

	if (!seccomp_may_assign_mode(seccomp_mode))
		goto out;

#ifdef TIF_NOTSC
	disable_TSC();
#endif
	seccomp_assign_mode(current, seccomp_mode, 0);
	ret = 0;

out:
	spin_unlock_irq(&current->sighand->siglock);

	return ret;
}

static inline long seccomp_set_mode_filter(unsigned int flags,
					   const char __user *filter)
{
	return -EINVAL;
}

static long seccomp_get_action_avail(const char __user *uaction)
{
	u32 action;

	if (copy_from_user(&action, uaction, sizeof(action)))
		return -EFAULT;

	switch (action) {
	case SECCOMP_RET_KILL_PROCESS:
	case SECCOMP_RET_KILL_THREAD:
	case SECCOMP_RET_TRAP:
	case SECCOMP_RET_ERRNO:
	case SECCOMP_RET_USER_NOTIF:
	case SECCOMP_RET_TRACE:
	case SECCOMP_RET_LOG:
	case SECCOMP_RET_ALLOW:
		break;
	default:
		return -EOPNOTSUPP;
	}

	return 0;
}

static long seccomp_get_notif_sizes(void __user *usizes)
{
	struct seccomp_notif_sizes sizes = {
		.seccomp_notif = sizeof(struct seccomp_notif),
		.seccomp_notif_resp = sizeof(struct seccomp_notif_resp),
		.seccomp_data = sizeof(struct seccomp_data),
	};

	if (copy_to_user(usizes, &sizes, sizeof(sizes)))
		return -EFAULT;

	return 0;
}

/* Common entry point for both prctl and syscall. */
static long do_seccomp(unsigned int op, unsigned int flags,
		       void __user *uargs)
{
	switch (op) {
	case SECCOMP_SET_MODE_STRICT:
		if (flags != 0 || uargs != NULL)
			return -EINVAL;
		return seccomp_set_mode_strict();
	case SECCOMP_SET_MODE_FILTER:
		return seccomp_set_mode_filter(flags, uargs);
	case SECCOMP_GET_ACTION_AVAIL:
		if (flags != 0)
			return -EINVAL;

		return seccomp_get_action_avail(uargs);
	case SECCOMP_GET_NOTIF_SIZES:
		if (flags != 0)
			return -EINVAL;

		return seccomp_get_notif_sizes(uargs);
	default:
		return -EINVAL;
	}
}

SYSCALL_DEFINE3(seccomp, unsigned int, op, unsigned int, flags,
			 void __user *, uargs)
{
	return do_seccomp(op, flags, uargs);
}

/**
 * prctl_set_seccomp: configures current->seccomp.mode
 * @seccomp_mode: requested mode to use
 * @filter: optional struct sock_fprog for use with SECCOMP_MODE_FILTER
 *
 * Returns 0 on success or -EINVAL on failure.
 */
long prctl_set_seccomp(unsigned long seccomp_mode, void __user *filter)
{
	unsigned int op;
	void __user *uargs;

	switch (seccomp_mode) {
	case SECCOMP_MODE_STRICT:
		op = SECCOMP_SET_MODE_STRICT;
		/*
		 * Setting strict mode through prctl always ignored filter,
		 * so make sure it is always NULL here to pass the internal
		 * check in do_seccomp().
		 */
		uargs = NULL;
		break;
	case SECCOMP_MODE_FILTER:
		op = SECCOMP_SET_MODE_FILTER;
		uargs = filter;
		break;
	default:
		return -EINVAL;
	}

	/* prctl interface doesn't have flags, so they are always zero. */
	return do_seccomp(op, 0, uargs);
}


#ifdef CONFIG_SYSCTL

/* Human readable action names for friendly sysctl interaction */
#define SECCOMP_RET_KILL_PROCESS_NAME	"kill_process"
#define SECCOMP_RET_KILL_THREAD_NAME	"kill_thread"
#define SECCOMP_RET_TRAP_NAME		"trap"
#define SECCOMP_RET_ERRNO_NAME		"errno"
#define SECCOMP_RET_USER_NOTIF_NAME	"user_notif"
#define SECCOMP_RET_TRACE_NAME		"trace"
#define SECCOMP_RET_LOG_NAME		"log"
#define SECCOMP_RET_ALLOW_NAME		"allow"

static const char seccomp_actions_avail[] =
				SECCOMP_RET_KILL_PROCESS_NAME	" "
				SECCOMP_RET_KILL_THREAD_NAME	" "
				SECCOMP_RET_TRAP_NAME		" "
				SECCOMP_RET_ERRNO_NAME		" "
				SECCOMP_RET_USER_NOTIF_NAME     " "
				SECCOMP_RET_TRACE_NAME		" "
				SECCOMP_RET_LOG_NAME		" "
				SECCOMP_RET_ALLOW_NAME;

struct seccomp_log_name {
	u32		log;
	const char	*name;
};

static const struct seccomp_log_name seccomp_log_names[] = {
	{ SECCOMP_LOG_KILL_PROCESS, SECCOMP_RET_KILL_PROCESS_NAME },
	{ SECCOMP_LOG_KILL_THREAD, SECCOMP_RET_KILL_THREAD_NAME },
	{ SECCOMP_LOG_TRAP, SECCOMP_RET_TRAP_NAME },
	{ SECCOMP_LOG_ERRNO, SECCOMP_RET_ERRNO_NAME },
	{ SECCOMP_LOG_USER_NOTIF, SECCOMP_RET_USER_NOTIF_NAME },
	{ SECCOMP_LOG_TRACE, SECCOMP_RET_TRACE_NAME },
	{ SECCOMP_LOG_LOG, SECCOMP_RET_LOG_NAME },
	{ SECCOMP_LOG_ALLOW, SECCOMP_RET_ALLOW_NAME },
	{ }
};

static bool seccomp_names_from_actions_logged(char *names, size_t size,
					      u32 actions_logged,
					      const char *sep)
{
	const struct seccomp_log_name *cur;
	bool append_sep = false;

	for (cur = seccomp_log_names; cur->name && size; cur++) {
		ssize_t ret;

		if (!(actions_logged & cur->log))
			continue;

		if (append_sep) {
			ret = strscpy(names, sep, size);
			if (ret < 0)
				return false;

			names += ret;
			size -= ret;
		} else
			append_sep = true;

		ret = strscpy(names, cur->name, size);
		if (ret < 0)
			return false;

		names += ret;
		size -= ret;
	}

	return true;
}

static bool seccomp_action_logged_from_name(u32 *action_logged,
					    const char *name)
{
	const struct seccomp_log_name *cur;

	for (cur = seccomp_log_names; cur->name; cur++) {
		if (!strcmp(cur->name, name)) {
			*action_logged = cur->log;
			return true;
		}
	}

	return false;
}

static bool seccomp_actions_logged_from_names(u32 *actions_logged, char *names)
{
	char *name;

	*actions_logged = 0;
	while ((name = strsep(&names, " ")) && *name) {
		u32 action_logged = 0;

		if (!seccomp_action_logged_from_name(&action_logged, name))
			return false;

		*actions_logged |= action_logged;
	}

	return true;
}

static int read_actions_logged(const struct ctl_table *ro_table, void *buffer,
			       size_t *lenp, loff_t *ppos)
{
	char names[sizeof(seccomp_actions_avail)];
	struct ctl_table table;

	memset(names, 0, sizeof(names));

	if (!seccomp_names_from_actions_logged(names, sizeof(names),
					       seccomp_actions_logged, " "))
		return -EINVAL;

	table = *ro_table;
	table.data = names;
	table.maxlen = sizeof(names);
	return proc_dostring(&table, 0, buffer, lenp, ppos);
}

static int write_actions_logged(const struct ctl_table *ro_table, void *buffer,
				size_t *lenp, loff_t *ppos, u32 *actions_logged)
{
	char names[sizeof(seccomp_actions_avail)];
	struct ctl_table table;
	int ret;

	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;

	memset(names, 0, sizeof(names));

	table = *ro_table;
	table.data = names;
	table.maxlen = sizeof(names);
	ret = proc_dostring(&table, 1, buffer, lenp, ppos);
	if (ret)
		return ret;

	if (!seccomp_actions_logged_from_names(actions_logged, table.data))
		return -EINVAL;

	if (*actions_logged & SECCOMP_LOG_ALLOW)
		return -EINVAL;

	seccomp_actions_logged = *actions_logged;
	return 0;
}

static void audit_actions_logged(u32 actions_logged, u32 old_actions_logged,
				 int ret)
{
	char names[sizeof(seccomp_actions_avail)];
	char old_names[sizeof(seccomp_actions_avail)];
	const char *new = names;
	const char *old = old_names;

	if (!audit_enabled)
		return;

	memset(names, 0, sizeof(names));
	memset(old_names, 0, sizeof(old_names));

	if (ret)
		new = "?";
	else if (!actions_logged)
		new = "(none)";
	else if (!seccomp_names_from_actions_logged(names, sizeof(names),
						    actions_logged, ","))
		new = "?";

	if (!old_actions_logged)
		old = "(none)";
	else if (!seccomp_names_from_actions_logged(old_names,
						    sizeof(old_names),
						    old_actions_logged, ","))
		old = "?";

	return audit_seccomp_actions_logged(new, old, !ret);
}

static int seccomp_actions_logged_handler(const struct ctl_table *ro_table, int write,
					  void *buffer, size_t *lenp,
					  loff_t *ppos)
{
	int ret;

	if (write) {
		u32 actions_logged = 0;
		u32 old_actions_logged = seccomp_actions_logged;

		ret = write_actions_logged(ro_table, buffer, lenp, ppos,
					   &actions_logged);
		audit_actions_logged(actions_logged, old_actions_logged, ret);
	} else
		ret = read_actions_logged(ro_table, buffer, lenp, ppos);

	return ret;
}

static const struct ctl_table seccomp_sysctl_table[] = {
	{
		.procname	= "actions_avail",
		.data		= (void *) &seccomp_actions_avail,
		.maxlen		= sizeof(seccomp_actions_avail),
		.mode		= 0444,
		.proc_handler	= proc_dostring,
	},
	{
		.procname	= "actions_logged",
		.mode		= 0644,
		.proc_handler	= seccomp_actions_logged_handler,
	},
};

static int __init seccomp_sysctl_init(void)
{
	register_sysctl_init("kernel/seccomp", seccomp_sysctl_table);
	return 0;
}

device_initcall(seccomp_sysctl_init)

#endif /* CONFIG_SYSCTL */
