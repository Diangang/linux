/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _KERNEL_STATS_H
#define _KERNEL_STATS_H

static inline void rq_sched_info_arrive  (struct rq *rq, unsigned long long delta) { }
static inline void rq_sched_info_dequeue(struct rq *rq, unsigned long long delta) { }
static inline void rq_sched_info_depart  (struct rq *rq, unsigned long long delta) { }
# define   schedstat_enabled()		0
# define __schedstat_inc(var)		do { } while (0)
# define   schedstat_inc(var)		do { } while (0)
# define __schedstat_add(var, amt)	do { } while (0)
# define   schedstat_add(var, amt)	do { } while (0)
# define __schedstat_set(var, val)	do { } while (0)
# define   schedstat_set(var, val)	do { } while (0)
# define   schedstat_val(var)		0
# define   schedstat_val_or_zero(var)	0

# define __update_stats_wait_start(rq, p, stats)       do { } while (0)
# define __update_stats_wait_end(rq, p, stats)         do { } while (0)
# define __update_stats_enqueue_sleeper(rq, p, stats)  do { } while (0)
# define check_schedstat_required()                    do { } while (0)


static inline struct sched_statistics *
__schedstats_from_se(struct sched_entity *se)
{
	return &task_of(se)->stats;
}

static inline void psi_enqueue(struct task_struct *p, bool migrate) {}
static inline void psi_dequeue(struct task_struct *p, bool migrate) {}
static inline void psi_ttwu_dequeue(struct task_struct *p) {}
static inline void psi_sched_switch(struct task_struct *prev,
				    struct task_struct *next,
				    bool sleep) {}
static inline void psi_account_irqtime(struct rq *rq, struct task_struct *curr,
				       struct task_struct *prev) {}

# define sched_info_enqueue(rq, t)	do { } while (0)
# define sched_info_dequeue(rq, t)	do { } while (0)
# define sched_info_switch(rq, t, next)	do { } while (0)

#endif /* _KERNEL_STATS_H */
