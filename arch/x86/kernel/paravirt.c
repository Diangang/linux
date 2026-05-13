// SPDX-License-Identifier: GPL-2.0-or-later
/*  Paravirtualization interfaces
    Copyright (C) 2006 Rusty Russell IBM Corporation


    2007 - x86_64 support added by Glauber de Oliveira Costa, Red Hat Inc
*/

#include <linux/errno.h>
#include <linux/init.h>
#include <linux/export.h>
#include <linux/efi.h>
#include <linux/bcd.h>
#include <linux/highmem.h>
#include <linux/kprobes.h>
#include <linux/pgtable.h>
#include <linux/static_call.h>

#include <asm/bug.h>
#include <asm/paravirt.h>
#include <asm/debugreg.h>
#include <asm/desc.h>
#include <asm/setup.h>
#include <asm/time.h>
#include <asm/pgalloc.h>
#include <asm/irq.h>
#include <asm/delay.h>
#include <asm/fixmap.h>
#include <asm/apic.h>
#include <asm/tlbflush.h>
#include <asm/timer.h>
#include <asm/special_insns.h>
#include <asm/tlb.h>
#include <asm/io_bitmap.h>
#include <asm/gsseg.h>
#include <asm/msr.h>

/* stub always returning 0. */
DEFINE_ASM_FUNC(paravirt_ret0, "xor %eax,%eax", .entry.text);

void __init default_banner(void)
{
	printk(KERN_INFO "Booting paravirtualized kernel on %s\n",
	       pv_info.name);
}


static noinstr void pv_native_safe_halt(void)
{
	native_safe_halt();
}


struct pv_info pv_info = {
	.name = "bare hardware",
	.io_delay = true,
};

/* 64-bit pagetable entries */
#define PTE_IDENT	__PV_IS_CALLEE_SAVE(_paravirt_ident_64)

struct paravirt_patch_template pv_ops = {
	/* Cpu ops. */

	/* Irq HLT ops. */
	.irq.safe_halt		= pv_native_safe_halt,
	.irq.halt		= native_halt,

	/* Mmu ops. */
	.mmu.flush_tlb_user	= native_flush_tlb_local,
	.mmu.flush_tlb_kernel	= native_flush_tlb_global,
	.mmu.flush_tlb_one_user	= native_flush_tlb_one_user,
	.mmu.flush_tlb_multi	= native_flush_tlb_multi,

	.mmu.exit_mmap		= paravirt_nop,
	.mmu.notify_page_enc_status_changed	= paravirt_nop,

};


EXPORT_SYMBOL(pv_ops);
EXPORT_SYMBOL_GPL(pv_info);
