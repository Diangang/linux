# SPDX-License-Identifier: GPL-2.0
# Kernel does not boot with instrumentation of tlb.c.
KCOV_INSTRUMENT_tlb.o			:= n
KCOV_INSTRUMENT_pgprot.o		:= n
# See the "Disable KCOV" comment in arch/x86/kernel/Makefile.
KCOV_INSTRUMENT_physaddr.o		:= n

KASAN_SANITIZE_pgprot.o		:= n

# Avoid recursion by not calling KMSAN hooks for CEA code.
KMSAN_SANITIZE_cpu_entry_area.o := n

obj-y				:=  init.o init_$(BITS).o fault.o ioremap.o extable.o mmap.o \
				    pgtable.o physaddr.o tlb.o cpu_entry_area.o maccess.o pgprot.o

obj-y				+= pat/

# Make sure __phys_addr has no stackprotector
CFLAGS_physaddr.o		:= -fno-stack-protector


obj-$(CONFIG_PTDUMP)		+= dump_pagetables.o


KMSAN_SANITIZE_kmsan_shadow.o	:= n


mmiotrace-y			:= kmmio.o pf_in.o mmio-mod.o


obj-$(CONFIG_NUMA)		+= numa.o
obj-$(CONFIG_AMD_NUMA)		+= amdtopology.o
obj-$(CONFIG_ACPI_NUMA)		+= srat.o

obj-$(CONFIG_X86_INTEL_MEMORY_PROTECTION_KEYS)	+= pkeys.o
obj-$(CONFIG_RANDOMIZE_MEMORY)			+= kaslr.o
obj-$(CONFIG_MITIGATION_PAGE_TABLE_ISOLATION)	+= pti.o
