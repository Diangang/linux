/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_SHM_H_
#define _LINUX_SHM_H_

#include <linux/types.h>
#include <asm/page.h>
#include <asm/shmparam.h>

struct file;
struct task_struct;

struct sysv_shm {
	/* empty */
};

static inline long do_shmat(int shmid, char __user *shmaddr,
			    int shmflg, unsigned long *addr,
			    unsigned long shmlba)
{
	return -ENOSYS;
}
static inline void exit_shm(struct task_struct *task)
{
}
static inline void shm_init_task(struct task_struct *task)
{
}

#endif /* _LINUX_SHM_H_ */
