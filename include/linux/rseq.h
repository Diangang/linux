/* SPDX-License-Identifier: GPL-2.0+ WITH Linux-syscall-note */
#ifndef _LINUX_RSEQ_H
#define _LINUX_RSEQ_H

static inline void rseq_handle_slowpath(struct pt_regs *regs) { }
static inline void rseq_signal_deliver(struct ksignal *ksig, struct pt_regs *regs) { }
static inline void rseq_sched_switch_event(struct task_struct *t) { }
static inline void rseq_sched_set_ids_changed(struct task_struct *t) { }
static inline void rseq_force_update(void) { }
static inline void rseq_virt_userspace_exit(void) { }
static inline void rseq_fork(struct task_struct *t, u64 clone_flags) { }
static inline void rseq_execve(struct task_struct *t) { }

static inline void rseq_syscall(struct pt_regs *regs) { }

static inline void rseq_syscall_enter_work(long syscall) { }
static inline int rseq_slice_extension_prctl(unsigned long arg2, unsigned long arg3)
{
	return -ENOTSUPP;
}

#endif /* _LINUX_RSEQ_H */
