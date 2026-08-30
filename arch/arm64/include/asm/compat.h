/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2012 ARM Ltd.
 */
#ifndef __ASM_COMPAT_H
#define __ASM_COMPAT_H

#define compat_mode_t compat_mode_t
typedef u16		compat_mode_t;

#define __compat_uid_t	__compat_uid_t
typedef u16		__compat_uid_t;
typedef u16		__compat_gid_t;

#define compat_ipc_pid_t compat_ipc_pid_t
typedef u16		compat_ipc_pid_t;

#define compat_statfs	compat_statfs

#include <asm-generic/compat.h>


static inline int is_compat_thread(struct thread_info *thread)
{
	return 0;
}

#endif /* __ASM_COMPAT_H */
