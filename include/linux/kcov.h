/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_KCOV_H
#define _LINUX_KCOV_H

#include <linux/sched.h>
#include <uapi/linux/kcov.h>

struct task_struct;


static inline void kcov_task_init(struct task_struct *t) {}
static inline void kcov_task_exit(struct task_struct *t) {}
static inline void kcov_prepare_switch(struct task_struct *t) {}
static inline void kcov_finish_switch(struct task_struct *t) {}
static inline void kcov_remote_start(u64 handle) {}
static inline void kcov_remote_stop(void) {}
static inline u64 kcov_common_handle(void)
{
	return 0;
}
static inline void kcov_remote_start_common(u64 id) {}
static inline void kcov_remote_start_usb(u64 id) {}
static inline void kcov_remote_start_usb_softirq(u64 id) {}
static inline void kcov_remote_stop_softirq(void) {}

#endif /* _LINUX_KCOV_H */
