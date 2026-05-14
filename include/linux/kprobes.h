/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef _LINUX_KPROBES_H
#define _LINUX_KPROBES_H

#include <linux/types.h>
#include <asm/kprobes.h>

struct pt_regs;

static inline bool kprobe_page_fault(struct pt_regs *regs, unsigned int trap)
{
	return false;
}

#endif /* _LINUX_KPROBES_H */
