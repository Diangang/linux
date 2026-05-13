// SPDX-License-Identifier: GPL-2.0
/*
 * Split spinlock implementation out into its own file, so it can be
 * compiled in a FTRACE-compatible way.
 */
#include <linux/static_call.h>
#include <linux/spinlock.h>
#include <linux/export.h>
#include <linux/jump_label.h>

DEFINE_STATIC_KEY_FALSE(virt_spin_lock_key);

#ifdef CONFIG_SMP
void __init native_pv_lock_init(void)
{
	if (boot_cpu_has(X86_FEATURE_HYPERVISOR))
		static_branch_enable(&virt_spin_lock_key);
}
#endif
