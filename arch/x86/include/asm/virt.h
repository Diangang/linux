/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _ASM_X86_VIRT_H
#define _ASM_X86_VIRT_H

#include <asm/reboot.h>

typedef void (cpu_emergency_virt_cb)(void);

static __always_inline void x86_virt_init(void) {}
static inline int x86_virt_emergency_disable_virtualization_cpu(void) { return -ENOENT; }

#endif /* _ASM_X86_VIRT_H */
