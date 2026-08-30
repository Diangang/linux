/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_PROFILE_H
#define _LINUX_PROFILE_H

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/cache.h>

#include <asm/errno.h>

#define CPU_PROFILING	1
#define SCHED_PROFILING	2
#define KVM_PROFILING	4

struct proc_dir_entry;
struct notifier_block;

static inline int create_proc_profile(void)
{
	return 0;
}


#define prof_on 0

static inline int profile_init(void)
{
	return 0;
}

static inline void profile_tick(int type)
{
	return;
}

static inline void profile_hits(int type, void *ip, unsigned int nr_hits)
{
	return;
}

static inline void profile_hit(int type, void *ip)
{
	return;
}



#endif /* _LINUX_PROFILE_H */
