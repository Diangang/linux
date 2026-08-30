/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Linker script variables to be set after section resolution, as
 * ld.lld does not like variables assigned before SECTIONS is processed.
 */
#ifndef __ARM64_KERNEL_IMAGE_VARS_H
#define __ARM64_KERNEL_IMAGE_VARS_H

#ifndef LINKER_SCRIPT
#error This file should only be included in vmlinux.lds.S
#endif


#define PI_EXPORT_SYM(sym)		\
	__PI_EXPORT_SYM(sym, __pi_ ## sym, Cannot export BSS symbol sym to startup code)
#define __PI_EXPORT_SYM(sym, pisym, msg)\
	PROVIDE(pisym = sym);		\
	ASSERT((sym - KIMAGE_VADDR) < (__bss_start - KIMAGE_VADDR), #msg)

PROVIDE(__efistub_primary_entry		= primary_entry);

/*
 * The EFI stub has its own symbol namespace prefixed by __efistub_, to
 * isolate it from the kernel proper. The following symbols are legally
 * accessed by the stub, so provide some aliases to make them accessible.
 * Only include data symbols here, or text symbols of functions that are
 * guaranteed to be safe when executed at another offset than they were
 * linked at. The routines below are all implemented in assembler in a
 * position independent manner
 */
PROVIDE(__efistub_caches_clean_inval_pou = __pi_caches_clean_inval_pou);

PROVIDE(__efistub__text			= _text);
PROVIDE(__efistub__end			= _end);
PROVIDE(__efistub___inittext_end       	= __inittext_end);
PROVIDE(__efistub__edata		= _edata);
PROVIDE(__efistub__ctype		= _ctype);

PROVIDE(__pi___memcpy			= __pi_memcpy);
PROVIDE(__pi___memmove			= __pi_memmove);
PROVIDE(__pi___memset			= __pi_memset);

PI_EXPORT_SYM(id_aa64isar1_override);
PI_EXPORT_SYM(id_aa64isar2_override);
PI_EXPORT_SYM(id_aa64mmfr0_override);
PI_EXPORT_SYM(id_aa64mmfr1_override);
PI_EXPORT_SYM(id_aa64mmfr2_override);
PI_EXPORT_SYM(id_aa64pfr0_override);
PI_EXPORT_SYM(id_aa64pfr1_override);
PI_EXPORT_SYM(id_aa64smfr0_override);
PI_EXPORT_SYM(id_aa64zfr0_override);
PI_EXPORT_SYM(arm64_sw_feature_override);
PI_EXPORT_SYM(arm64_use_ng_mappings);
PI_EXPORT_SYM(_ctype);

PI_EXPORT_SYM(swapper_pg_dir);

PI_EXPORT_SYM(_text);
PI_EXPORT_SYM(_stext);
PI_EXPORT_SYM(_etext);
PI_EXPORT_SYM(__start_rodata);
PI_EXPORT_SYM(__inittext_begin);
PI_EXPORT_SYM(__inittext_end);
PI_EXPORT_SYM(__initdata_begin);
PI_EXPORT_SYM(__initdata_end);
PI_EXPORT_SYM(_data);


/*
 * LLD will occasionally error out with a '__init_end does not converge' error
 * if INIT_IDMAP_DIR_SIZE is defined in terms of _end, as this results in a
 * circular dependency. Counter this by dimensioning the initial IDMAP page
 * tables based on kimage_limit, which is defined such that its value should
 * not change as a result of the initdata segment being pushed over a 64k
 * segment boundary due to changes in INIT_IDMAP_DIR_SIZE, provided that its
 * value doesn't change by more than 2M between linker passes.
 */
kimage_limit = ALIGN(ABSOLUTE(_end + SZ_64K), SZ_2M);

#undef ASSERT

#endif /* __ARM64_KERNEL_IMAGE_VARS_H */
