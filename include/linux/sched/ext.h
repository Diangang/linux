/* SPDX-License-Identifier: GPL-2.0 */
/*
 * BPF extensible scheduler class: Documentation/scheduler/sched-ext.rst
 *
 * Copyright (c) 2022 Meta Platforms, Inc. and affiliates.
 * Copyright (c) 2022 Tejun Heo <tj@kernel.org>
 * Copyright (c) 2022 David Vernet <dvernet@meta.com>
 */
#ifndef _LINUX_SCHED_EXT_H
#define _LINUX_SCHED_EXT_H


static inline void sched_ext_dead(struct task_struct *p) {}
static inline void print_scx_info(const char *log_lvl, struct task_struct *p) {}
static inline void scx_softlockup(u32 dur_s) {}
static inline bool scx_hardlockup(int cpu) { return false; }
static inline bool scx_rcu_cpu_stall(void) { return false; }


struct scx_task_group {
#ifdef CONFIG_EXT_GROUP_SCHED
	u32			flags;		/* SCX_TG_* */
	u32			weight;
	u64			bw_period_us;
	u64			bw_quota_us;
	u64			bw_burst_us;
	bool			idle;
#endif
};

#endif	/* _LINUX_SCHED_EXT_H */
