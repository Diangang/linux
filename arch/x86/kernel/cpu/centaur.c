// SPDX-License-Identifier: GPL-2.0

#include <linux/sched.h>
#include <linux/sched/clock.h>

#include <asm/cpu.h>
#include <asm/cpufeature.h>
#include <asm/e820/api.h>
#include <asm/mtrr.h>
#include <asm/msr.h>

#include "cpu.h"

#define ACE_PRESENT	(1 << 6)
#define ACE_ENABLED	(1 << 7)
#define ACE_FCR		(1 << 28)	/* MSR_VIA_FCR */

#define RNG_PRESENT	(1 << 2)
#define RNG_ENABLED	(1 << 3)
#define RNG_ENABLE	(1 << 6)	/* MSR_VIA_RNG */

static void init_c3(struct cpuinfo_x86 *c)
{
	u32  lo, hi;

	/* Test for Centaur Extended Feature Flags presence */
	if (cpuid_eax(0xC0000000) >= 0xC0000001) {
		u32 tmp = cpuid_edx(0xC0000001);

		/* enable ACE unit, if present and disabled */
		if ((tmp & (ACE_PRESENT | ACE_ENABLED)) == ACE_PRESENT) {
			rdmsr(MSR_VIA_FCR, lo, hi);
			lo |= ACE_FCR;		/* enable ACE unit */
			wrmsr(MSR_VIA_FCR, lo, hi);
			pr_info("CPU: Enabled ACE h/w crypto\n");
		}

		/* enable RNG unit, if present and disabled */
		if ((tmp & (RNG_PRESENT | RNG_ENABLED)) == RNG_PRESENT) {
			rdmsr(MSR_VIA_RNG, lo, hi);
			lo |= RNG_ENABLE;	/* enable RNG unit */
			wrmsr(MSR_VIA_RNG, lo, hi);
			pr_info("CPU: Enabled h/w RNG\n");
		}

		/* store Centaur Extended Feature Flags as
		 * word 5 of the CPU capability bit array
		 */
		c->x86_capability[CPUID_C000_0001_EDX] = cpuid_edx(0xC0000001);
	}
	if (c->x86 == 0x6 && c->x86_model >= 0xf) {
		c->x86_cache_alignment = c->x86_clflush_size * 2;
		set_cpu_cap(c, X86_FEATURE_REP_GOOD);
	}

	if (c->x86 >= 7)
		set_cpu_cap(c, X86_FEATURE_REP_GOOD);
}

enum {
		ECX8		= 1<<1,
		EIERRINT	= 1<<2,
		DPM		= 1<<3,
		DMCE		= 1<<4,
		DSTPCLK		= 1<<5,
		ELINEAR		= 1<<6,
		DSMC		= 1<<7,
		DTLOCK		= 1<<8,
		EDCTLB		= 1<<8,
		EMMX		= 1<<9,
		DPDC		= 1<<11,
		EBRPRED		= 1<<12,
		DIC		= 1<<13,
		DDC		= 1<<14,
		DNA		= 1<<15,
		ERETSTK		= 1<<16,
		E2MMX		= 1<<19,
		EAMD3D		= 1<<20,
};

static void early_init_centaur(struct cpuinfo_x86 *c)
{
	if ((c->x86 == 6 && c->x86_model >= 0xf) ||
	    (c->x86 >= 7))
		set_cpu_cap(c, X86_FEATURE_CONSTANT_TSC);

	if (c->x86_power & (1 << 8)) {
		set_cpu_cap(c, X86_FEATURE_CONSTANT_TSC);
		set_cpu_cap(c, X86_FEATURE_NONSTOP_TSC);
	}
}

static void init_centaur(struct cpuinfo_x86 *c)
{
	early_init_centaur(c);
	init_intel_cacheinfo(c);

	if (c->cpuid_level > 9) {
		unsigned int eax = cpuid_eax(10);

		/*
		 * Check for version and the number of counters
		 * Version(eax[7:0]) can't be 0;
		 * Counters(eax[15:8]) should be greater than 1;
		 */
		if ((eax & 0xff) && (((eax >> 8) & 0xff) > 1))
			set_cpu_cap(c, X86_FEATURE_ARCH_PERFMON);
	}

	if (c->x86 == 6 || c->x86 >= 7)
		init_c3(c);
#ifdef CONFIG_X86_64
	set_cpu_cap(c, X86_FEATURE_LFENCE_RDTSC);
#endif

	init_ia32_feat_ctl(c);
}


static const struct cpu_dev centaur_cpu_dev = {
	.c_vendor	= "Centaur",
	.c_ident	= { "CentaurHauls" },
	.c_early_init	= early_init_centaur,
	.c_init		= init_centaur,
	.c_x86_vendor	= X86_VENDOR_CENTAUR,
};

cpu_dev_register(centaur_cpu_dev);
