/* SPDX-License-Identifier: GPL-2.0 */
/*
 * KMSAN checks to be used for one-off annotations in subsystems.
 *
 * Copyright (C) 2017-2022 Google LLC
 * Author: Alexander Potapenko <glider@google.com>
 *
 */

#ifndef _LINUX_KMSAN_CHECKS_H
#define _LINUX_KMSAN_CHECKS_H

#include <linux/types.h>


static inline void kmsan_poison_memory(const void *address, size_t size,
				       gfp_t flags)
{
}
static inline void kmsan_unpoison_memory(const void *address, size_t size)
{
}
static inline void kmsan_check_memory(const void *address, size_t size)
{
}
static inline void kmsan_copy_to_user(void __user *to, const void *from,
				      size_t to_copy, size_t left)
{
}

static inline void kmsan_memmove(void *to, const void *from, size_t to_copy)
{
}


#endif /* _LINUX_KMSAN_CHECKS_H */
