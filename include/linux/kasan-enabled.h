/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_KASAN_ENABLED_H
#define _LINUX_KASAN_ENABLED_H

#include <linux/static_key.h>

#if 0
/*
 * Global runtime flag for KASAN modes that need runtime control.
 * Used by HW_TAGS mode.
 */
DECLARE_STATIC_KEY_FALSE(kasan_flag_enabled);

/*
 * Runtime control for HW_TAGS mode.
 */
static __always_inline bool kasan_enabled(void)
{
	return static_branch_likely(&kasan_flag_enabled);
}

static inline void kasan_enable(void)
{
	static_branch_enable(&kasan_flag_enabled);
}
#else
/* For architectures that can enable KASAN early, use compile-time check. */
static __always_inline bool kasan_enabled(void)
{
	return IS_ENABLED(CONFIG_KASAN);
}

static inline void kasan_enable(void) {}
#endif /* CONFIG_KASAN_HW_TAGS */

#if 0
static inline bool kasan_hw_tags_enabled(void)
{
	return kasan_enabled();
}
#else
static inline bool kasan_hw_tags_enabled(void)
{
	return false;
}
#endif /* CONFIG_KASAN_HW_TAGS */

#endif /* LINUX_KASAN_ENABLED_H */
