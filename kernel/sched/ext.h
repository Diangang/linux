/* SPDX-License-Identifier: GPL-2.0 */
/*
 * BPF extensible scheduler class: Documentation/scheduler/sched-ext.rst
 *
 * Copyright (c) 2022 Meta Platforms, Inc. and affiliates.
 * Copyright (c) 2022 Tejun Heo <tj@kernel.org>
 * Copyright (c) 2022 David Vernet <dvernet@meta.com>
 */

static inline void scx_tick(struct rq *rq) {}
static inline void scx_pre_fork(struct task_struct *p) {}
static inline int scx_fork(struct task_struct *p, struct kernel_clone_args *kargs) { return 0; }
static inline void scx_post_fork(struct task_struct *p) {}
static inline void scx_cancel_fork(struct task_struct *p) {}
static inline u32 scx_cpuperf_target(s32 cpu) { return 0; }
static inline bool scx_can_stop_tick(struct rq *rq) { return true; }
static inline void scx_rq_activate(struct rq *rq) {}
static inline void scx_rq_deactivate(struct rq *rq) {}
static inline int scx_check_setscheduler(struct task_struct *p, int policy) { return 0; }
static inline bool task_on_scx(const struct task_struct *p) { return false; }
static inline bool scx_allow_ttwu_queue(const struct task_struct *p) { return true; }
static inline void init_sched_ext_class(void) {}


static inline void scx_update_idle(struct rq *rq, bool idle, bool do_notify) {}
