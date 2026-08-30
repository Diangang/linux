/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_ARM64_HYPERVISOR_H
#define _ASM_ARM64_HYPERVISOR_H

static inline void pkvm_init_hyp_services(void) { };

static inline void kvm_arch_init_hyp_services(void)
{
	pkvm_init_hyp_services();
};

#endif
