/* SPDX-License-Identifier: GPL-2.0 */
#ifndef LINUX_CRASH_DUMP_H
#define LINUX_CRASH_DUMP_H

#include <linux/types.h>

static inline bool is_kdump_kernel(void) { return false; }

#endif /* LINUX_CRASH_DUMP_H */
