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
__is_affected_midr_range(const struct arm64_cpu_capabilities *entry,
			 u32 midr, u32 revidr)
{
	const struct arm64_midr_revidr *fix;
	if (!is_midr_in_range(&entry->midr_range))
		return false;

	midr &= MIDR_REVISION_MASK | MIDR_VARIANT_MASK;
	for (fix = entry->fixed_revs; fix && fix->revidr_mask; fix++)
		if (midr == fix->midr_rv && (revidr & fix->revidr_mask))
			return false;
	return true;
}

static bool __maybe_unused
is_affected_midr_range(const struct arm64_cpu_capabilities *entry, int scope)
{
	int i;

	if (!target_impl_cpu_num) {
		WARN_ON(scope != SCOPE_LOCAL_CPU || preemptible());
		return __is_affected_midr_range(entry, read_cpuid_id(),
						read_cpuid(REVIDR_EL1));
	}

	for (i = 0; i < target_impl_cpu_num; i++) {
		if (__is_affected_midr_range(entry, target_impl_cpus[i].midr,
					     target_impl_cpus[i].midr))
			return true;
	}
	return false;
}

static bool __maybe_unused
is_affected_midr_range_list(const struct arm64_cpu_capabilities *entry,
			    int scope)
{
	WARN_ON(scope != SCOPE_LOCAL_CPU || preemptible());
	return is_midr_in_range_list(entry->midr_range_list);
}

static bool __maybe_unused
is_kryo_midr(const struct arm64_cpu_capabilities *entry, int scope)
{
	u32 model;

	WARN_ON(scope != SCOPE_LOCAL_CPU || preemptible());

	model = read_cpuid_id();
	model &= MIDR_IMPLEMENTOR_MASK | (0xf00 << MIDR_PARTNUM_SHIFT) |
		 MIDR_ARCHITECTURE_MASK;

	return model == entry->midr_range.model;
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


static void __maybe_unused
cpu_enable_cache_maint_trap(const struct arm64_cpu_capabilities *__unused)
{
	sysreg_clear_set(sctlr_el1, SCTLR_EL1_UCI, 0);
}

#define CAP_MIDR_RANGE(model, v_min, r_min, v_max, r_max)	\
	.matches = is_affected_midr_range,			\
	.midr_range = MIDR_RANGE(model, v_min, r_min, v_max, r_max)

#define CAP_MIDR_ALL_VERSIONS(model)					\
	.matches = is_affected_midr_range,				\
	.midr_range = MIDR_ALL_VERSIONS(model)

#define MIDR_FIXED(rev, revidr_mask) \
	.fixed_revs = (struct arm64_midr_revidr[]){{ (rev), (revidr_mask) }, {}}

#define ERRATA_MIDR_RANGE(model, v_min, r_min, v_max, r_max)		\
	.type = ARM64_CPUCAP_LOCAL_CPU_ERRATUM,				\
	CAP_MIDR_RANGE(model, v_min, r_min, v_max, r_max)

#define CAP_MIDR_RANGE_LIST(list)				\
	.matches = is_affected_midr_range_list,			\
	.midr_range_list = list

/* Errata affecting a range of revisions of  given model variant */
#define ERRATA_MIDR_REV_RANGE(m, var, r_min, r_max)	 \
	ERRATA_MIDR_RANGE(m, var, r_min, var, r_max)

/* Errata affecting a single variant/revision of a model */
#define ERRATA_MIDR_REV(model, var, rev)	\
	ERRATA_MIDR_RANGE(model, var, rev, var, rev)

/* Errata affecting all variants/revisions of a given a model */
#define ERRATA_MIDR_ALL_VERSIONS(model)				\
	.type = ARM64_CPUCAP_LOCAL_CPU_ERRATUM,			\
	CAP_MIDR_ALL_VERSIONS(model)

/* Errata affecting a list of midr ranges, with same work around */
#define ERRATA_MIDR_RANGE_LIST(midr_list)			\
	.type = ARM64_CPUCAP_LOCAL_CPU_ERRATUM,			\
	CAP_MIDR_RANGE_LIST(midr_list)

static const __maybe_unused struct midr_range tx2_family_cpus[] = {
	MIDR_ALL_VERSIONS(MIDR_BRCM_VULCAN),
	MIDR_ALL_VERSIONS(MIDR_CAVIUM_THUNDERX2),
	{},
};

static bool __maybe_unused
needs_tx2_tvm_workaround(const struct arm64_cpu_capabilities *entry,
			 int scope)
{
	int i;

	if (!is_affected_midr_range_list(entry, scope) ||
	    !is_hyp_mode_available())
		return false;

	for_each_possible_cpu(i) {
		if (MPIDR_AFFINITY_LEVEL(cpu_logical_map(i), 0) != 0)
			return true;
	}

	return false;
}

static bool __maybe_unused
has_neoverse_n1_erratum_1542419(const struct arm64_cpu_capabilities *entry,
				int scope)
{
	bool has_dic = read_cpuid_cachetype() & BIT(CTR_EL0_DIC_SHIFT);
	const struct midr_range range = MIDR_ALL_VERSIONS(MIDR_NEOVERSE_N1);

	WARN_ON(scope != SCOPE_LOCAL_CPU || preemptible());
	return is_midr_in_range(&range) && has_dic;
}

#if 0
static const struct midr_range trbe_overwrite_fill_mode_cpus[] = {
#ifdef CONFIG_ARM64_ERRATUM_2139208
	MIDR_ALL_VERSIONS(MIDR_NEOVERSE_N2),
	MIDR_ALL_VERSIONS(MIDR_MICROSOFT_AZURE_COBALT_100),
#endif
#ifdef CONFIG_ARM64_ERRATUM_2119858
	MIDR_ALL_VERSIONS(MIDR_CORTEX_A710),
	MIDR_RANGE(MIDR_CORTEX_X2, 0, 0, 2, 0),
#endif
	{},
};
#endif	/* CONFIG_ARM64_WORKAROUND_TRBE_OVERWRITE_FILL_MODE */


#if 0
static struct midr_range trbe_write_out_of_range_cpus[] = {
#ifdef CONFIG_ARM64_ERRATUM_2253138
	MIDR_ALL_VERSIONS(MIDR_NEOVERSE_N2),
	MIDR_ALL_VERSIONS(MIDR_MICROSOFT_AZURE_COBALT_100),
#endif
#ifdef CONFIG_ARM64_ERRATUM_2224489
	MIDR_ALL_VERSIONS(MIDR_CORTEX_A710),
	MIDR_RANGE(MIDR_CORTEX_X2, 0, 0, 2, 0),
#endif
	{},
};
#endif /* CONFIG_ARM64_WORKAROUND_TRBE_WRITE_OUT_OF_RANGE */





const struct arm64_cpu_capabilities arm64_errata[] = {
#ifdef CONFIG_ARM64_ERRATUM_834220
	{
	/* Cortex-A57 r0p0 - r1p2 */
		.desc = "ARM erratum 834220",
		.capability = ARM64_WORKAROUND_834220,
		ERRATA_MIDR_RANGE(MIDR_CORTEX_A57,
				  0, 0,
				  1, 2),
	},
#endif
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
#ifdef CONFIG_RANDOMIZE_BASE
	{
	/* Must come after the Spectre-v2 entry */
		.desc = "Spectre-v3a",
		.capability = ARM64_SPECTRE_V3A,
		.type = ARM64_CPUCAP_LOCAL_CPU_ERRATUM,
		.matches = has_spectre_v3a,
		.cpu_enable = spectre_v3a_enable_mitigation,
	},
#endif
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
#ifdef CONFIG_ARM64_ERRATUM_1542419
	{
		/* we depend on the firmware portion for correctness */
		.desc = "ARM erratum 1542419 (kernel portion)",
		.capability = ARM64_WORKAROUND_1542419,
		.type = ARM64_CPUCAP_LOCAL_CPU_ERRATUM,
		.matches = has_neoverse_n1_erratum_1542419,
		.cpu_enable = cpu_enable_trap_ctr_access,
	},
#endif
#if 0
	{
		/*
		 * The erratum work around is handled within the TRBE
		 * driver and can be applied per-cpu. So, we can allow
		 * a late CPU to come online with this erratum.
		 */
		.desc = "ARM erratum 2119858 or 2139208",
		.capability = ARM64_WORKAROUND_TRBE_OVERWRITE_FILL_MODE,
		.type = ARM64_CPUCAP_WEAK_LOCAL_CPU_FEATURE,
		CAP_MIDR_RANGE_LIST(trbe_overwrite_fill_mode_cpus),
	},
#endif
#if 0
	{
		.desc = "ARM erratum 2253138 or 2224489",
		.capability = ARM64_WORKAROUND_TRBE_WRITE_OUT_OF_RANGE,
		.type = ARM64_CPUCAP_WEAK_LOCAL_CPU_FEATURE,
		CAP_MIDR_RANGE_LIST(trbe_write_out_of_range_cpus),
	},
#endif
#ifdef CONFIG_ARM64_ERRATUM_2064142
	{
		.desc = "ARM erratum 2064142",
		.capability = ARM64_WORKAROUND_2064142,

		/* Cortex-A510 r0p0 - r0p2 */
		ERRATA_MIDR_REV_RANGE(MIDR_CORTEX_A510, 0, 0, 2)
	},
#endif
#ifdef CONFIG_ARM64_ERRATUM_2038923
	{
		.desc = "ARM erratum 2038923",
		.capability = ARM64_WORKAROUND_2038923,

		/* Cortex-A510 r0p0 - r0p2 */
		ERRATA_MIDR_REV_RANGE(MIDR_CORTEX_A510, 0, 0, 2)
	},
#endif
#ifdef CONFIG_ARM64_ERRATUM_1902691
	{
		.desc = "ARM erratum 1902691",
		.capability = ARM64_WORKAROUND_1902691,

		/* Cortex-A510 r0p0 - r0p1 */
		ERRATA_MIDR_REV_RANGE(MIDR_CORTEX_A510, 0, 0, 1)
	},
#endif
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
