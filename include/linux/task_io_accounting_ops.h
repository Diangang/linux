/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Task I/O accounting operations
 */
#ifndef __TASK_IO_ACCOUNTING_OPS_INCLUDED
#define __TASK_IO_ACCOUNTING_OPS_INCLUDED

#include <linux/sched.h>


static inline void task_io_account_read(size_t bytes)
{
}

static inline unsigned long task_io_get_inblock(const struct task_struct *p)
{
	return 0;
}

static inline void task_io_account_write(size_t bytes)
{
}

static inline unsigned long task_io_get_oublock(const struct task_struct *p)
{
	return 0;
}

static inline void task_io_account_cancelled_write(size_t bytes)
{
}

static inline void task_io_accounting_init(struct task_io_accounting *ioac)
{
}

static inline void task_blk_io_accounting_add(struct task_io_accounting *dst,
						struct task_io_accounting *src)
{
}


static inline void task_chr_io_accounting_add(struct task_io_accounting *dst,
						struct task_io_accounting *src)
{
}

static inline void task_io_accounting_add(struct task_io_accounting *dst,
						struct task_io_accounting *src)
{
	task_chr_io_accounting_add(dst, src);
	task_blk_io_accounting_add(dst, src);
}
#endif /* __TASK_IO_ACCOUNTING_OPS_INCLUDED */
