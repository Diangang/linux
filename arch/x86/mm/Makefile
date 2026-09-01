# SPDX-License-Identifier: GPL-2.0
# Kernel does not boot with instrumentation of tlb.c.
# See the "Disable KCOV" comment in arch/x86/kernel/Makefile.

obj-y				:=  init.o init_$(BITS).o fault.o ioremap.o extable.o mmap.o \
				    pgtable.o physaddr.o tlb.o cpu_entry_area.o maccess.o pgprot.o

obj-y				+= pat/

# Make sure __phys_addr has no stackprotector
CFLAGS_physaddr.o		:= -fno-stack-protector




obj-$(CONFIG_NUMA)		+= numa.o
obj-$(CONFIG_AMD_NUMA)		+= amdtopology.o

obj-$(CONFIG_X86_INTEL_MEMORY_PROTECTION_KEYS)	+= pkeys.o
obj-$(CONFIG_RANDOMIZE_MEMORY)			+= kaslr.o
obj-$(CONFIG_MITIGATION_PAGE_TABLE_ISOLATION)	+= pti.o
