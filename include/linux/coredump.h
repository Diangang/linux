/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_COREDUMP_H
#define _LINUX_COREDUMP_H

#include <linux/types.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <asm/siginfo.h>

static inline void vfs_coredump(const kernel_siginfo_t *siginfo) {}

#define coredump_report(...)
#define coredump_report_failure(...)


static inline void validate_coredump_safety(void) {}

#endif /* _LINUX_COREDUMP_H */
