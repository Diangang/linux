/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_X86_UV_UV_H
#define _ASM_X86_UV_UV_H

enum uv_system_type {UV_NONE, UV_LEGACY_APIC, UV_X2APIC};


static inline enum uv_system_type get_uv_system_type(void) { return UV_NONE; }
static inline bool is_early_uv_system(void)	{ return 0; }
static inline int is_uv_system(void)	{ return 0; }
static inline int is_uv_hubbed(int uv)	{ return 0; }
static inline void uv_cpu_init(void)	{ }
static inline void uv_system_init(void)	{ }


#endif	/* _ASM_X86_UV_UV_H */
