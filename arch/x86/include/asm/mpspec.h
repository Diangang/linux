/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_X86_MPSPEC_H
#define _ASM_X86_MPSPEC_H

#include <linux/types.h>

#include <asm/mpspec_def.h>
#include <asm/x86_init.h>
#include <asm/apicdef.h>

extern int pic_mode;


#define MAX_MP_BUSSES		256
/* Each PCI slot may be a combo card with its own bus.  4 IRQ pins per slot. */
#define MAX_IRQ_SOURCES		(MAX_MP_BUSSES * 4)


extern DECLARE_BITMAP(mp_bus_not_pci, MAX_MP_BUSSES);

extern u32 boot_cpu_physical_apicid;
extern u8 boot_cpu_apic_version;

#ifdef CONFIG_X86_LOCAL_APIC
extern int smp_found_config;
#else
# define smp_found_config 0
#endif

#ifdef CONFIG_X86_MPPARSE
extern void e820__memblock_alloc_reserved_mpc_new(void);
extern int enable_update_mptable;
extern void mpparse_find_mptable(void);
extern void mpparse_parse_early_smp_config(void);
extern void mpparse_parse_smp_config(void);
#else
static inline void e820__memblock_alloc_reserved_mpc_new(void) { }
#define enable_update_mptable		0
#define mpparse_find_mptable		x86_init_noop
#define mpparse_parse_early_smp_config	x86_init_noop
#define mpparse_parse_smp_config	x86_init_noop
#endif

extern DECLARE_BITMAP(phys_cpu_present_map, MAX_LOCAL_APIC);

static inline void reset_phys_cpu_present_map(u32 apicid)
{
	bitmap_zero(phys_cpu_present_map, MAX_LOCAL_APIC);
	set_bit(apicid, phys_cpu_present_map);
}

static inline void copy_phys_cpu_present_map(unsigned long *dst)
{
	bitmap_copy(dst, phys_cpu_present_map, MAX_LOCAL_APIC);
}

#endif /* _ASM_X86_MPSPEC_H */
