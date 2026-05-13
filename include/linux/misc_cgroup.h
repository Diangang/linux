/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Miscellaneous cgroup controller.
 *
 * Copyright 2020 Google LLC
 * Author: Vipin Sharma <vipinsh@google.com>
 */
#ifndef _MISC_CGROUP_H_
#define _MISC_CGROUP_H_

/**
 * enum misc_res_type - Types of misc cgroup entries supported by the host.
 */
enum misc_res_type {
#ifdef CONFIG_KVM_AMD_SEV
	/** @MISC_CG_RES_SEV: AMD SEV ASIDs resource */
	MISC_CG_RES_SEV,
	/** @MISC_CG_RES_SEV_ES: AMD SEV-ES ASIDs resource */
	MISC_CG_RES_SEV_ES,
#endif
#ifdef CONFIG_INTEL_TDX_HOST
	/** @MISC_CG_RES_TDX: Intel TDX HKIDs resource */
	MISC_CG_RES_TDX,
#endif
	/** @MISC_CG_RES_TYPES: count of enum misc_res_type constants */
	MISC_CG_RES_TYPES
};

struct misc_cg;

static inline int misc_cg_set_capacity(enum misc_res_type type, u64 capacity)
{
	return 0;
}

static inline int misc_cg_try_charge(enum misc_res_type type,
				     struct misc_cg *cg,
				     u64 amount)
{
	return 0;
}

static inline void misc_cg_uncharge(enum misc_res_type type,
				    struct misc_cg *cg,
				    u64 amount)
{
}

static inline struct misc_cg *get_current_misc_cg(void)
{
	return NULL;
}

static inline void put_misc_cg(struct misc_cg *cg)
{
}

#endif /* _MISC_CGROUP_H_ */
