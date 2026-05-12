/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_TRACE_RECURSION_H
#define _LINUX_TRACE_RECURSION_H

#include <linux/interrupt.h>
#include <linux/sched.h>

static __always_inline int ftrace_test_recursion_trylock(unsigned long ip,
							 unsigned long parent_ip)
{
	return 0;
}

static __always_inline void ftrace_test_recursion_unlock(int bit)
{
}
#endif /* _LINUX_TRACE_RECURSION_H */
