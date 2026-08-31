/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_X86_EFI_H
#define _ASM_X86_EFI_H

#include <asm/fpu/api.h>
#include <asm/processor-flags.h>
#include <asm/tlb.h>
#include <asm/nospec-branch.h>
#include <asm/mmu_context.h>
#include <asm/ibt.h>
#include <linux/build_bug.h>
#include <linux/kernel.h>
#include <linux/pgtable.h>

extern unsigned long efi_fw_vendor, efi_config_table;
extern unsigned long efi_mixed_mode_stack_pa;

/*
 * We map the EFI regions needed for runtime services non-contiguously,
 * with preserved alignment on virtual addresses starting from -4G down
 * for a total max space of 64G. This way, we provide for stable runtime
 * services addresses across kernels so that a kexec'd kernel can still
 * use them.
 *
 * This is the main reason why we're doing stable VA mappings for RT
 * services.
 */

#define EFI32_LOADER_SIGNATURE	"EL32"
#define EFI64_LOADER_SIGNATURE	"EL64"

#define ARCH_EFI_IRQ_FLAGS_MASK	X86_EFLAGS_IF

#define EFI_UNACCEPTED_UNIT_SIZE PMD_SIZE

/*
 * The EFI services are called through variadic functions in many cases. These
 * functions are implemented in assembler and support only a fixed number of
 * arguments. The macros below allows us to check at build time that we don't
 * try to call them with too many arguments.
 *
 * __efi_nargs() will return the number of arguments if it is 7 or less, and
 * cause a BUILD_BUG otherwise. The limitations of the C preprocessor make it
 * impossible to calculate the exact number of arguments beyond some
 * pre-defined limit. The maximum number of arguments currently supported by
 * any of the thunks is 7, so this is good enough for now and can be extended
 * in the obvious way if we ever need more.
 */

#define __efi_nargs(...) __efi_nargs_(__VA_ARGS__)
#define __efi_nargs_(...) __efi_nargs__(0, ##__VA_ARGS__,	\
	__efi_arg_sentinel(9), __efi_arg_sentinel(8),		\
	__efi_arg_sentinel(7), __efi_arg_sentinel(6),		\
	__efi_arg_sentinel(5), __efi_arg_sentinel(4),		\
	__efi_arg_sentinel(3), __efi_arg_sentinel(2),		\
	__efi_arg_sentinel(1), __efi_arg_sentinel(0))
#define __efi_nargs__(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, n, ...)	\
	__take_second_arg(n,					\
		({ BUILD_BUG_ON_MSG(1, "__efi_nargs limit exceeded"); 10; }))
#define __efi_arg_sentinel(n) , n

/*
 * __efi_nargs_check(f, n, ...) will cause a BUILD_BUG if the ellipsis
 * represents more than n arguments.
 */

#define __efi_nargs_check(f, n, ...)					\
	__efi_nargs_check_(f, __efi_nargs(__VA_ARGS__), n)
#define __efi_nargs_check_(f, p, n) __efi_nargs_check__(f, p, n)
#define __efi_nargs_check__(f, p, n) ({					\
	BUILD_BUG_ON_MSG(						\
		(p) > (n),						\
		#f " called with too many arguments (" #p ">" #n ")");	\
})

static inline void efi_fpu_begin(void)
{
	/*
	 * The UEFI calling convention (UEFI spec 2.3.2 and 2.3.4) requires
	 * that FCW and MXCSR (64-bit) must be initialized prior to calling
	 * UEFI code.  (Oddly the spec does not require that the FPU stack
	 * be empty.)
	 */
	kernel_fpu_begin_mask(KFPU_387 | KFPU_MXCSR);
}

static inline void efi_fpu_end(void)
{
	kernel_fpu_end();
}

#define EFI_X86_KERNEL_ALLOC_LIMIT		EFI_ALLOC_LIMIT

extern asmlinkage u64 __efi_call(void *fp, ...);

#define efi_call(...) ({						\
	__efi_nargs_check(efi_call, 7, __VA_ARGS__);			\
	__efi_call(__VA_ARGS__);					\
})

#undef arch_efi_call_virt
#define arch_efi_call_virt(p, f, args...)	efi_call((void *)p->f, args)

extern int __init efi_memblock_x86_reserve_range(void);
extern void __init efi_print_memmap(void);
extern void __init efi_map_region(efi_memory_desc_t *md);
extern void __init efi_map_region_fixed(efi_memory_desc_t *md);
extern void efi_sync_low_kernel_mappings(void);
extern int __init efi_alloc_page_tables(void);
extern int __init efi_setup_page_tables(unsigned long pa_memmap, unsigned num_pages);
extern void __init efi_runtime_update_mappings(void);
extern void __init efi_dump_pagetable(void);
extern void __init efi_apply_memmap_quirks(void);
extern int __init efi_reuse_config(u64 tables, int nr_tables);
extern void efi_delete_dummy_variable(void);
extern void efi_crash_gracefully_on_page_fault(unsigned long phys_addr);
extern void efi_unmap_boot_services(void);

void arch_efi_call_virt_setup(void);
void arch_efi_call_virt_teardown(void);

extern u64 efi_setup;

static inline void parse_efi_setup(u64 phys_addr, u32 data_len) {}
static inline bool efi_reboot_required(void)
{
	return false;
}
static inline  bool efi_is_table_address(unsigned long phys_addr)
{
	return false;
}
static inline void efi_reserve_boot_services(void)
{
}

extern int __init efi_memmap_alloc(unsigned int num_entries,
				   struct efi_memory_map_data *data);

extern int __init efi_memmap_install(struct efi_memory_map_data *data);
extern int __init efi_memmap_split_count(efi_memory_desc_t *md,
					 struct range *range);
extern void __init efi_memmap_insert(struct efi_memory_map *old_memmap,
				     void *buf, struct efi_mem_range *mem);

enum efi_secureboot_mode __x86_efi_boot_mode(void);

#define arch_efi_boot_mode __x86_efi_boot_mode()

static inline int efi_get_runtime_map_size(void)
{
	return 0;
}

static inline int efi_get_runtime_map_desc_size(void)
{
	return 0;
}

static inline int efi_runtime_map_copy(void *buf, size_t bufsz)
{
	return 0;
}


#endif /* _ASM_X86_EFI_H */
