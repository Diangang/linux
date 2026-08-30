/* SPDX-License-Identifier: GPL-2.0 */

#ifndef _LINUX_BTF_IDS_H
#define _LINUX_BTF_IDS_H

#include <linux/types.h> /* for u32 */

struct btf_id_set {
	u32 cnt;
	u32 ids[];
};

/* This flag implies BTF_SET8 holds kfunc(s) */
#define BTF_SET8_KFUNCS		(1 << 0)

struct btf_id_set8 {
	u32 cnt;
	u32 flags;
	struct {
		u32 id;
		u32 flags;
	} pairs[];
};


#define BTF_ID_LIST(name) static u32 __maybe_unused name[128];
#define BTF_ID(prefix, name)
#define BTF_ID_FLAGS(prefix, name, ...)
#define BTF_ID_UNUSED
#define BTF_ID_LIST_GLOBAL(name, n) u32 __maybe_unused name[n];
#define BTF_ID_LIST_SINGLE(name, prefix, typename) static u32 __maybe_unused name[1];
#define BTF_ID_LIST_GLOBAL_SINGLE(name, prefix, typename) u32 __maybe_unused name[1];
#define BTF_SET_START(name) static struct btf_id_set __maybe_unused name = { 0 };
#define BTF_SET_START_GLOBAL(name) static struct btf_id_set __maybe_unused name = { 0 };
#define BTF_SET_END(name)
#define BTF_SET8_START(name) static struct btf_id_set8 __maybe_unused name = { 0 };
#define BTF_SET8_END(name)
#define BTF_KFUNCS_START(name) static struct btf_id_set8 __maybe_unused name = { .flags = BTF_SET8_KFUNCS };
#define BTF_KFUNCS_END(name)



#define BTF_TRACING_TYPE_xxx	\
	BTF_TRACING_TYPE(BTF_TRACING_TYPE_TASK, task_struct)	\
	BTF_TRACING_TYPE(BTF_TRACING_TYPE_FILE, file)		\
	BTF_TRACING_TYPE(BTF_TRACING_TYPE_VMA, vm_area_struct)

enum {
#define BTF_TRACING_TYPE(name, type) name,
BTF_TRACING_TYPE_xxx
#undef BTF_TRACING_TYPE
MAX_BTF_TRACING_TYPE,
};

extern u32 btf_tracing_ids[];
extern u32 bpf_cgroup_btf_id[];
extern u32 bpf_local_storage_map_btf_id[];
extern u32 btf_bpf_map_id[];
extern u32 bpf_kmem_cache_btf_id[];

#endif
