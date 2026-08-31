/* SPDX-License-Identifier: GPL-2.0 */
#ifndef LINUX_CRASH_RESERVE_H
#define LINUX_CRASH_RESERVE_H

#include <linux/init.h>

static inline void __init reserve_crashkernel_generic(
		unsigned long long crash_size,
		unsigned long long crash_base,
		unsigned long long crash_low_size,
		bool high)
{}
#endif /* LINUX_CRASH_RESERVE_H */
