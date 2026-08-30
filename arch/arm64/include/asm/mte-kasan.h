/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2020 ARM Ltd.
 */
#ifndef __ASM_MTE_KASAN_H
#define __ASM_MTE_KASAN_H

#include <asm/compiler.h>
#include <asm/cputype.h>
#include <asm/mte-def.h>

#ifndef __ASSEMBLER__

#include <linux/types.h>


static inline bool system_uses_mte_async_or_asymm_mode(void)
{
	return false;
}


static inline void mte_disable_tco(void)
{
}

static inline void mte_enable_tco(void)
{
}

static inline void __mte_disable_tco_async(void)
{
}

static inline void __mte_enable_tco_async(void)
{
}

static inline u8 mte_get_ptr_tag(void *ptr)
{
	return 0xFF;
}

static inline u8 mte_get_mem_tag(void *addr)
{
	return 0xFF;
}

static inline u8 mte_get_random_tag(void)
{
	return 0xFF;
}

static inline void mte_set_mem_tag_range(void *addr, size_t size,
						u8 tag, bool init)
{
}

static inline void mte_enable_kernel_sync(void)
{
}

static inline void mte_enable_kernel_async(void)
{
}

static inline void mte_enable_kernel_asymm(void)
{
}

static inline int mte_enable_kernel_store_only(void)
{
	return -EINVAL;
}

#endif /* __ASSEMBLER__ */

#endif /* __ASM_MTE_KASAN_H  */
