// SPDX-License-Identifier: GPL-2.0-only
/*
 * Contains CPU specific errata definitions
 *
 * Copyright (C) 2014 ARM Ltd.
 */

#include <linux/arm-smccc.h>
#include <linux/types.h>
#include <linux/cpu.h>
#include <asm/cpu.h>
#include <asm/cputype.h>
#include <asm/cpufeature.h>
#include <asm/fpsimd.h>
#include <asm/smp_plat.h>

static u64 target_impl_cpu_num;
static struct target_impl_cpu *target_impl_cpus;

bool cpu_errata_set_target_impl(u64 num, void *impl_cpus)
{
	if (target_impl_cpu_num || !num || !impl_cpus)
		return false;

	target_impl_cpu_num = num;
	target_impl_cpus = impl_cpus;
	return true;
}

static inline bool is_midr_in_range(struct midr_range const *range)
{
	int i;

	if (!target_impl_cpu_num)
		return midr_is_cpu_model_range(read_cpuid_id(), range->model,
					       range->rv_min, range->rv_max);

	for (i = 0; i < target_impl_cpu_num; i++) {
		if (midr_is_cpu_model_range(target_impl_cpus[i].midr,
					    range->model,
					    range->rv_min, range->rv_max))
			return true;
	}
	return false;
}

bool is_midr_in_range_list(struct midr_range const *ranges)
{
	while (ranges->model)
		if (is_midr_in_range(ranges++))
			return true;
	return false;
}
EXPORT_SYMBOL_GPL(is_midr_in_range_list);

static bool __maybe_unused
is_affected_midr_range_list(const struct arm64_cpu_capabilities *entry,
			    int scope)
{
	WARN_ON(scope != SCOPE_LOCAL_CPU || preemptible());
	return is_midr_in_range_list(entry->midr_range_list);
}

static bool
has_mismatched_cache_type(const struct arm64_cpu_capabilities *entry,
			  int scope)
{
	u64 mask = arm64_ftr_reg_ctrel0.strict_mask;
	u64 sys = arm64_ftr_reg_ctrel0.sys_val & mask;
	u64 ctr_raw, ctr_real;

	WARN_ON(scope != SCOPE_LOCAL_CPU || preemptible());

	/*
	 * We want to make sure that all the CPUs in the system expose
	 * a consistent CTR_EL0 to make sure that applications behaves
	 * correctly with migration.
	 *
	 * If a CPU has CTR_EL0.IDC but does not advertise it via CTR_EL0 :
	 *
	 * 1) It is safe if the system doesn't support IDC, as CPU anyway
	 *    reports IDC = 0, consistent with the rest.
	 *
	 * 2) If the system has IDC, it is still safe as we trap CTR_EL0
	 *    access on this CPU via the ARM64_HAS_CACHE_IDC capability.
	 *
	 * So, we need to make sure either the raw CTR_EL0 or the effective
	 * CTR_EL0 matches the system's copy to allow a secondary CPU to boot.
	 */
	ctr_raw = read_cpuid_cachetype() & mask;
	ctr_real = read_cpuid_effective_cachetype() & mask;

	return (ctr_real != sys) && (ctr_raw != sys);
}


static void
cpu_enable_trap_ctr_access(const struct arm64_cpu_capabilities *cap)
{
	u64 mask = arm64_ftr_reg_ctrel0.strict_mask;
	bool enable_uct_trap = false;

	/* Trap CTR_EL0 access on this CPU, only if it has a mismatch */
	if ((read_cpuid_cachetype() & mask) !=
	    (arm64_ftr_reg_ctrel0.sys_val & mask))
		enable_uct_trap = true;

	/* ... or if the system is affected by an erratum */
	if (cap->capability == ARM64_WORKAROUND_1542419)
		enable_uct_trap = true;

	if (enable_uct_trap)
		sysreg_clear_set(sctlr_el1, SCTLR_EL1_UCT, 0);
}


#define CAP_MIDR_RANGE_LIST(list)				\
	.matches = is_affected_midr_range_list,			\
	.midr_range_list = list

/* Errata affecting a list of midr ranges, with same work around */
#define ERRATA_MIDR_RANGE_LIST(midr_list)			\
	.type = ARM64_CPUCAP_LOCAL_CPU_ERRATUM,			\
	CAP_MIDR_RANGE_LIST(midr_list)

const struct arm64_cpu_capabilities arm64_errata[] = {
	{
		.desc = "Mismatched cache type (CTR_EL0)",
		.capability = ARM64_MISMATCHED_CACHE_TYPE,
		.matches = has_mismatched_cache_type,
		.type = ARM64_CPUCAP_LOCAL_CPU_ERRATUM,
		.cpu_enable = cpu_enable_trap_ctr_access,
	},
	{
		.desc = "Spectre-v2",
		.capability = ARM64_SPECTRE_V2,
		.type = ARM64_CPUCAP_LOCAL_CPU_ERRATUM,
		.matches = has_spectre_v2,
		.cpu_enable = spectre_v2_enable_mitigation,
	},
	{
		.desc = "Spectre-v4",
		.capability = ARM64_SPECTRE_V4,
		.type = ARM64_CPUCAP_LOCAL_CPU_ERRATUM,
		.matches = has_spectre_v4,
		.cpu_enable = spectre_v4_enable_mitigation,
	},
	{
		.desc = "Spectre-BHB",
		.capability = ARM64_SPECTRE_BHB,
		.type = ARM64_CPUCAP_LOCAL_CPU_ERRATUM,
		.matches = is_spectre_bhb_affected,
		.cpu_enable = spectre_bhb_enable_mitigation,
	},
	{
		.desc = "Broken CNTVOFF_EL2",
		.capability = ARM64_WORKAROUND_QCOM_ORYON_CNTVOFF,
		ERRATA_MIDR_RANGE_LIST(((const struct midr_range[]) {
					MIDR_ALL_VERSIONS(MIDR_QCOM_ORYON_X1),
					{}
				})),
	},
	{
	}
};
