/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __ACPI_PROCESSOR_H
#define __ACPI_PROCESSOR_H

#include <linux/cpu.h>
#include <linux/cpufreq.h>
#include <linux/pm_qos.h>
#include <linux/printk.h>
#include <linux/sched.h>
#include <linux/smp.h>
#include <linux/thermal.h>
#include <linux/types.h>
#include <linux/workqueue.h>

#include <asm/acpi.h>

#define ACPI_PROCESSOR_DEVICE_HID	"ACPI0007"
#define ACPI_PROCESSOR_CONTAINER_HID	"ACPI0010"

#define ACPI_PROCESSOR_BUSY_METRIC	10

#define ACPI_PROCESSOR_MAX_POWER	8
#define ACPI_PROCESSOR_MAX_C2_LATENCY	100
#define ACPI_PROCESSOR_MAX_C3_LATENCY	1000

#define ACPI_PROCESSOR_MAX_THROTTLING	16
#define ACPI_PROCESSOR_MAX_THROTTLE	250	/* 25% */
#define ACPI_PROCESSOR_MAX_DUTY_WIDTH	4

#define ACPI_PDC_REVISION_ID		0x1

#define ACPI_PSD_REV0_REVISION		0	/* Support for _PSD as in ACPI 3.0 */
#define ACPI_PSD_REV0_ENTRIES		5

#define ACPI_TSD_REV0_REVISION		0	/* Support for _PSD as in ACPI 3.0 */
#define ACPI_TSD_REV0_ENTRIES		5
/*
 * Types of coordination defined in ACPI 3.0. Same macros can be used across
 * P, C and T states
 */
#define DOMAIN_COORD_TYPE_SW_ALL	0xfc
#define DOMAIN_COORD_TYPE_SW_ANY	0xfd
#define DOMAIN_COORD_TYPE_HW_ALL	0xfe

#define ACPI_CSTATE_SYSTEMIO	0
#define ACPI_CSTATE_FFH		1
#define ACPI_CSTATE_HALT	2
#define ACPI_CSTATE_INTEGER	3

#define ACPI_CX_DESC_LEN	32

/* Power Management */

struct acpi_processor_cx;

struct acpi_power_register {
	u8 descriptor;
	u16 length;
	u8 space_id;
	u8 bit_width;
	u8 bit_offset;
	u8 access_size;
	u64 address;
} __packed;

struct acpi_processor_cx {
	u8 valid;
	u8 type;
	u32 address;
	u8 entry_method;
	u8 index;
	u32 latency;
	u8 bm_sts_skip;
	char desc[ACPI_CX_DESC_LEN];
};

struct acpi_lpi_state {
	u32 min_residency;
	u32 wake_latency; /* worst case */
	u32 flags;
	u32 arch_flags;
	u32 res_cnt_freq;
	u32 enable_parent_state;
	u64 address;
	u8 index;
	u8 entry_method;
	char desc[ACPI_CX_DESC_LEN];
};

struct acpi_processor_power {
	int count;
	union {
		struct acpi_processor_cx states[ACPI_PROCESSOR_MAX_POWER];
		struct acpi_lpi_state lpi_states[ACPI_PROCESSOR_MAX_POWER];
	};
	int timer_broadcast_on_state;
};

/* Performance Management */

struct acpi_psd_package {
	u64 num_entries;
	u64 revision;
	u64 domain;
	u64 coord_type;
	u64 num_processors;
} __packed;

struct acpi_pct_register {
	u8 descriptor;
	u16 length;
	u8 space_id;
	u8 bit_width;
	u8 bit_offset;
	u8 reserved;
	u64 address;
} __packed;

struct acpi_processor_px {
	u64 core_frequency;	/* megahertz */
	u64 power;	/* milliWatts */
	u64 transition_latency;	/* microseconds */
	u64 bus_master_latency;	/* microseconds */
	u64 control;	/* control value */
	u64 status;	/* success indicator */
};

struct acpi_processor_performance {
	unsigned int state;
	unsigned int platform_limit;
	struct acpi_pct_register control_register;
	struct acpi_pct_register status_register;
	unsigned int state_count;
	struct acpi_processor_px *states;
	struct acpi_psd_package domain_info;
	cpumask_var_t shared_cpu_map;
	unsigned int shared_type;
};

/* Throttling Control */

struct acpi_tsd_package {
	u64 num_entries;
	u64 revision;
	u64 domain;
	u64 coord_type;
	u64 num_processors;
} __packed;

struct acpi_ptc_register {
	u8 descriptor;
	u16 length;
	u8 space_id;
	u8 bit_width;
	u8 bit_offset;
	u8 reserved;
	u64 address;
} __packed;

struct acpi_processor_tx_tss {
	u64 freqpercentage;	/* */
	u64 power;	/* milliWatts */
	u64 transition_latency;	/* microseconds */
	u64 control;	/* control value */
	u64 status;	/* success indicator */
};
struct acpi_processor_tx {
	u16 power;
	u16 performance;
};

struct acpi_processor;
struct acpi_processor_throttling {
	unsigned int state;
	unsigned int platform_limit;
	struct acpi_pct_register control_register;
	struct acpi_pct_register status_register;
	unsigned int state_count;
	struct acpi_processor_tx_tss *states_tss;
	struct acpi_tsd_package domain_info;
	cpumask_var_t shared_cpu_map;
	int (*acpi_processor_get_throttling) (struct acpi_processor * pr);

	u32 address;
	u8 duty_offset;
	u8 duty_width;
	u8 tsd_valid_flag;
	unsigned int shared_type;
	struct acpi_processor_tx states[ACPI_PROCESSOR_MAX_THROTTLING];
};

/* Limit Interface */

struct acpi_processor_lx {
	int px;			/* performance state */
	int tx;			/* throttle level */
};

struct acpi_processor_limit {
	struct acpi_processor_lx state;	/* current limit */
	struct acpi_processor_lx thermal;	/* thermal limit */
	struct acpi_processor_lx user;	/* user limit */
};

struct acpi_processor_flags {
	u8 power:1;
	u8 performance:1;
	u8 throttling:1;
	u8 limit:1;
	u8 bm_control:1;
	u8 bm_check:1;
	u8 has_cst:1;
	u8 has_lpi:1;
	u8 power_setup_done:1;
	u8 bm_rld_set:1;
	u8 previously_online:1;
};

struct acpi_processor {
	acpi_handle handle;
	u32 acpi_id;
	phys_cpuid_t phys_id;	/* CPU hardware ID such as APIC ID for x86 */
	u32 id;		/* CPU logical ID allocated by OS */
	u32 pblk;
	int performance_platform_limit;
	int throttling_platform_limit;
	/* 0 - states 0..n-th state available */

	struct acpi_processor_flags flags;
	struct acpi_processor_power power;
	struct acpi_processor_performance *performance;
	struct acpi_processor_throttling throttling;
	struct acpi_processor_limit limit;
	struct thermal_cooling_device *cdev;
	struct device *dev; /* Processor device. */
	struct freq_qos_request perflib_req;
	struct freq_qos_request thermal_req;
};

struct acpi_processor_errata {
	u8 smp;
	struct {
		u8 throttle:1;
		u8 fdma:1;
		u8 reserved:6;
		u32 bmisx;
	} piix4;
};

extern int acpi_processor_preregister_performance(struct
						  acpi_processor_performance
						  __percpu *performance);

extern int acpi_processor_register_performance(struct acpi_processor_performance
					       *performance, unsigned int cpu);
extern void acpi_processor_unregister_performance(unsigned int cpu);

int acpi_processor_pstate_control(void);
/* note: this locks both the calling module and the processor module
         if a _PPC object exists, rmmod is disallowed then */
int acpi_processor_notify_smm(struct module *calling_module);
int acpi_processor_get_psd(acpi_handle handle,
			   struct acpi_psd_package *pdomain);

/* parsing the _P* objects. */
extern int acpi_processor_get_performance_info(struct acpi_processor *pr);

/* for communication between multiple parts of the processor kernel module */
DECLARE_PER_CPU(struct acpi_processor *, processors);
extern struct acpi_processor_errata errata;

static inline void __noreturn acpi_processor_ffh_play_dead(struct acpi_processor_cx *cx)
{
	BUG();
}

/* in processor_perflib.c */

/* in processor_core.c */
phys_cpuid_t acpi_get_phys_id(acpi_handle, int type, u32 acpi_id);
phys_cpuid_t acpi_map_madt_entry(u32 acpi_id);
int acpi_map_cpuid(phys_cpuid_t phys_id, u32 acpi_id);
int acpi_get_cpuid(acpi_handle, int type, u32 acpi_id);

/* in processor_pdc.c */
void acpi_processor_set_pdc(acpi_handle handle);

/* in processor_throttling.c */
/* in processor_idle.c */

/* in processor_thermal.c */
int acpi_processor_thermal_init(struct acpi_processor *pr,
				struct acpi_device *device);
void acpi_processor_thermal_exit(struct acpi_processor *pr,
				 struct acpi_device *device);
extern const struct thermal_cooling_device_ops processor_cooling_ops;
void acpi_processor_init_invariance_cppc(void);

#endif
