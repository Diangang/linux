/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_KASAN_ENABLED_H
#define _LINUX_KASAN_ENABLED_H

static __always_inline bool kasan_enabled(void)
{
	return false;
}

static inline void kasan_enable(void) {}

static inline bool kasan_hw_tags_enabled(void)
{
	return false;
}

#endif /* LINUX_KASAN_ENABLED_H */
