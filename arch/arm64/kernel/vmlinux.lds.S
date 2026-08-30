/* SPDX-License-Identifier: GPL-2.0 */
/*
 * ld script to make ARM Linux kernel
 * taken from the i386 version by Russell King
 * Written by Martin Mares <mj@atrey.karlin.mff.cuni.cz>
 */

#include <asm/hyp_image.h>
#define HYPERVISOR_EXTABLE
#define HYPERVISOR_RODATA_SECTIONS
#define HYPERVISOR_DATA_SECTION
#define HYPERVISOR_PERCPU_SECTION
#define HYPERVISOR_RELOC_SECTION
#define SBSS_ALIGN			0

#define RO_EXCEPTION_TABLE_ALIGN	4
#define RUNTIME_DISCARD_EXIT

#include <asm-generic/vmlinux.lds.h>
#include <asm/cache.h>
#include <asm/kernel-pgtable.h>
#include <asm/memory.h>
#include <asm/page.h>

#include "image.h"

OUTPUT_ARCH(aarch64)
ENTRY(_text)

jiffies = jiffies_64;

#define HYPERVISOR_TEXT					\
	. = ALIGN(PAGE_SIZE);				\
	__hyp_idmap_text_start = .;			\
	*(.hyp.idmap.text)				\
	__hyp_idmap_text_end = .;			\
	__hyp_text_start = .;				\
	*(.hyp.text)					\
	HYPERVISOR_EXTABLE				\
	. = ALIGN(PAGE_SIZE);				\
	__hyp_text_end = .;

#define IDMAP_TEXT					\
	. = ALIGN(SZ_4K);				\
	__idmap_text_start = .;				\
	*(.idmap.text)					\
	__idmap_text_end = .;

#define KEXEC_TEXT

#ifdef CONFIG_UNMAP_KERNEL_AT_EL0
#define TRAMP_TEXT					\
	. = ALIGN(PAGE_SIZE);				\
	__entry_tramp_text_start = .;			\
	*(.entry.tramp.text)				\
	. = ALIGN(PAGE_SIZE);				\
	__entry_tramp_text_end = .;			\
	*(.entry.tramp.rodata)
#else
#define TRAMP_TEXT
#endif

#define UNWIND_DATA_SECTIONS

/*
 * The size of the PE/COFF section that covers the kernel image, which
 * runs from _stext to _edata, must be a round multiple of the PE/COFF
 * FileAlignment, which we set to its minimum value of 0x200. '_stext'
 * itself is 4 KB aligned, so padding out _edata to a 0x200 aligned
 * boundary should be sufficient.
 */
PECOFF_FILE_ALIGNMENT = 0x200;

#define PECOFF_EDATA_PADDING

SECTIONS
{
	/*
	 * XXX: The linker does not define how output sections are
	 * assigned to input sections when there are multiple statements
	 * matching the same input section name.  There is no documented
	 * order of matching.
	 */
	DISCARDS
	/DISCARD/ : {
		*(.interp .dynamic)
		*(.dynsym .dynstr .hash .gnu.hash)
		*(.ARM.attributes)
	}

	. = KIMAGE_VADDR;

	.head.text : {
		_text = .;
		HEAD_TEXT
	}
	.text : ALIGN(SEGMENT_ALIGN) {	/* Real text segment		*/
		_stext = .;		/* Text and read-only data	*/
			IRQENTRY_TEXT
			SOFTIRQENTRY_TEXT
			ENTRY_TEXT
			TEXT_TEXT
			SCHED_TEXT
			LOCK_TEXT
			HYPERVISOR_TEXT
			STATIC_CALL_TEXT
			*(.gnu.warning)
	}

	. = ALIGN(SEGMENT_ALIGN);
	_etext = .;			/* End of text section */

	/* everything from this point to __init_begin will be marked RO NX */
	RO_DATA(PAGE_SIZE)

	HYPERVISOR_RODATA_SECTIONS

	.got : { *(.got) }
	/*
	 * Make sure that the .got.plt is either completely empty or it
	 * contains only the lazy dispatch entries.
	 */
	.got.plt : { *(.got.plt) }
	ASSERT(SIZEOF(.got.plt) == 0 || SIZEOF(.got.plt) == 0x18,
	       "Unexpected GOT/PLT entries detected!")

	/* code sections that are never executed via the kernel mapping */
	.rodata.text : {
		TRAMP_TEXT
		KEXEC_TEXT
		IDMAP_TEXT
		. = ALIGN(PAGE_SIZE);
	}

	idmap_pg_dir = .;
	. += PAGE_SIZE;

#ifdef CONFIG_UNMAP_KERNEL_AT_EL0
	tramp_pg_dir = .;
	. += PAGE_SIZE;
#endif

	reserved_pg_dir = .;
	. += PAGE_SIZE;

	swapper_pg_dir = .;
	. += PAGE_SIZE;

	. = ALIGN(SEGMENT_ALIGN);
	__init_begin = .;
	__inittext_begin = .;

	INIT_TEXT_SECTION(8)

	__exittext_begin = .;
	.exit.text : {
		EXIT_TEXT
	}
	__exittext_end = .;

	. = ALIGN(4);
	.altinstructions : {
		__alt_instructions = .;
		*(.altinstructions)
		__alt_instructions_end = .;
	}

	UNWIND_DATA_SECTIONS

	. = ALIGN(SEGMENT_ALIGN);
	__inittext_end = .;
	__initdata_begin = .;

	__pi_init_idmap_pg_dir = .;
	. += INIT_IDMAP_DIR_SIZE;
	__pi_init_idmap_pg_end = .;

	.init.data : {
		INIT_DATA
		INIT_SETUP(16)
		INIT_CALLS
		CON_INITCALL
		INIT_RAM_FS
		*(.init.altinstructions .init.bss)	/* from the EFI stub */
	}
	.exit.data : {
		EXIT_DATA
	}

	RUNTIME_CONST_VARIABLES

	PERCPU_SECTION(L1_CACHE_BYTES)
	HYPERVISOR_PERCPU_SECTION

	HYPERVISOR_RELOC_SECTION

	.rela.dyn : ALIGN(8) {
		__pi_rela_start = .;
		*(.rela .rela*)
		__pi_rela_end = .;
	}

	.relr.dyn : ALIGN(8) {
		__pi_relr_start = .;
		*(.relr.dyn)
		__pi_relr_end = .;
	}

	. = ALIGN(SEGMENT_ALIGN);
	__initdata_end = .;
	__init_end = .;

	.data.rel.ro : { *(.data.rel.ro) }
	ASSERT(SIZEOF(.data.rel.ro) == 0, "Unexpected RELRO detected!")

	_data = .;
	_sdata = .;
	RW_DATA(L1_CACHE_BYTES, PAGE_SIZE, THREAD_ALIGN)

	HYPERVISOR_DATA_SECTION

	/*
	 * Data written with the MMU off but read with the MMU on requires
	 * cache lines to be invalidated, discarding up to a Cache Writeback
	 * Granule (CWG) of data from the cache. Keep the section that
	 * requires this type of maintenance to be in its own Cache Writeback
	 * Granule (CWG) area so the cache maintenance operations don't
	 * interfere with adjacent data.
	 */
	.mmuoff.data.write : ALIGN(SZ_2K) {
		__mmuoff_data_start = .;
		*(.mmuoff.data.write)
	}
	. = ALIGN(SZ_2K);
	.mmuoff.data.read : {
		*(.mmuoff.data.read)
		__mmuoff_data_end = .;
	}

	PECOFF_EDATA_PADDING
	__pecoff_data_rawsize = ABSOLUTE(. - __initdata_begin);
	_edata = .;

	/* start of zero-init region */
	BSS_SECTION(SBSS_ALIGN, 0, 0)
	__pi___bss_start = __bss_start;

	. = ALIGN(PAGE_SIZE);
	__pi_init_pg_dir = .;
	. += INIT_DIR_SIZE;
	__pi_init_pg_end = .;
	/* end of zero-init region */

	. += SZ_4K;		/* stack for the early C runtime */
	early_init_stack = .;

	. = ALIGN(SEGMENT_ALIGN);
	__pecoff_data_size = ABSOLUTE(. - __initdata_begin);
	_end = .;
	__pi__end = .;

	STABS_DEBUG
	DWARF_DEBUG
	MODINFO
	ELF_DETAILS

	HEAD_SYMBOLS

	/*
	 * Sections that should stay zero sized, which is safer to
	 * explicitly check instead of blindly discarding.
	 */
	.plt : {
		*(.plt) *(.plt.*) *(.iplt) *(.igot .igot.plt)
	}
	ASSERT(SIZEOF(.plt) == 0, "Unexpected run-time procedure linkages detected!")
}

#include "image-vars.h"

/*
 * The HYP init code and ID map text can't be longer than a page each. The
 * former is page-aligned, but the latter may not be with 16K or 64K pages, so
 * it should also not cross a page boundary.
 */
ASSERT(__hyp_idmap_text_end - __hyp_idmap_text_start <= PAGE_SIZE,
	"HYP init code too big")
ASSERT(__idmap_text_end - (__idmap_text_start & ~(SZ_4K - 1)) <= SZ_4K,
	"ID map text too big or misaligned")
#ifdef CONFIG_UNMAP_KERNEL_AT_EL0
ASSERT((__entry_tramp_text_end - __entry_tramp_text_start) <= 3*PAGE_SIZE,
	"Entry trampoline text too big")
#endif
/*
 * If padding is applied before .head.text, virt<->phys conversions will fail.
 */
ASSERT(_text == KIMAGE_VADDR, "HEAD is misaligned")

ASSERT(swapper_pg_dir - reserved_pg_dir == RESERVED_SWAPPER_OFFSET,
       "RESERVED_SWAPPER_OFFSET is wrong!")

#ifdef CONFIG_UNMAP_KERNEL_AT_EL0
ASSERT(swapper_pg_dir - tramp_pg_dir == TRAMP_SWAPPER_OFFSET,
       "TRAMP_SWAPPER_OFFSET is wrong!")
#endif
