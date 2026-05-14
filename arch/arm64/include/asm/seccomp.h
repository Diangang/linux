/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * arch/arm64/include/asm/seccomp.h
 *
 * Copyright (C) 2014 Linaro Limited
 * Author: AKASHI Takahiro <takahiro.akashi@linaro.org>
 */
#ifndef _ASM_SECCOMP_H
#define _ASM_SECCOMP_H

#include <asm-generic/seccomp.h>

#define SECCOMP_ARCH_NATIVE		AUDIT_ARCH_AARCH64
#define SECCOMP_ARCH_NATIVE_NR		NR_syscalls
#define SECCOMP_ARCH_NATIVE_NAME	"aarch64"

#endif /* _ASM_SECCOMP_H */
