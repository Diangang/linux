# SPDX-License-Identifier: GPL-2.0
#
# Makefile for the linux kernel.
#

always-$(KBUILD_BUILTIN)	+= vmlinux.lds

CPPFLAGS_vmlinux.lds += -U$(UTS_MACHINE)

# With some compiler versions the generated code results in boot hangs, caused
# by several compilation units. To be safe, disable all instrumentation.

# If instrumentation of the following files is enabled, boot hangs during
# first second.
# These are called from save_stack_trace() on debug paths,
# and produce large amounts of uninteresting coverage.
CFLAGS_head64.o := -fno-stack-protector

obj-y			+= head_$(BITS).o
obj-y			+= head$(BITS).o
obj-y			+= ebda.o
obj-y			+= platform-quirks.o
obj-y			+= process_$(BITS).o signal.o signal_$(BITS).o
obj-y			+= traps.o idt.o irq.o irq_$(BITS).o dumpstack_$(BITS).o
obj-y			+= time.o dumpstack.o nmi.o
obj-y			+= setup.o x86_init.o i8259.o irqinit.o
obj-$(CONFIG_JUMP_LABEL)	+= jump_label.o
obj-$(CONFIG_IRQ_WORK)  += irq_work.o
obj-y			+= probe_roms.o
obj-$(CONFIG_X86_64)	+= sys_x86_64.o
obj-$(CONFIG_SYSFS)	+= ksysfs.o
obj-y			+= bootflag.o e820.o
obj-y			+= pci-dma.o quirks.o kdebugfs.o
obj-y			+= alternative.o i8253.o
obj-y			+= tsc.o tsc_msr.o io_delay.o rtc.o
obj-y			+= resource.o
obj-y			+= irqflags.o
obj-y			+= static_call.o

obj-y				+= process.o
obj-y				+= fpu/
obj-y				+= ptrace.o
obj-y				+= tls.o
obj-y				+= step.o
obj-$(CONFIG_ISA_DMA_API)	+= i8237.o
obj-y				+= stacktrace.o
obj-y				+= cpu/
obj-y				+= acpi/
obj-y				+= reboot.o
obj-$(CONFIG_PCI)		+= early-quirks.o
obj-$(CONFIG_SMP)		+= smp.o
obj-$(CONFIG_SMP)		+= smpboot.o
obj-$(CONFIG_X86_TSC)		+= tsc_sync.o
obj-$(CONFIG_SMP)		+= setup_percpu.o
obj-y				+= apic/
obj-$(CONFIG_HPET_TIMER) 	+= hpet.o

obj-$(CONFIG_AMD_NB)		+= amd_nb.o
obj-$(CONFIG_AMD_NODE)		+= amd_node.o


obj-$(CONFIG_UNWINDER_ORC)		+= unwind_orc.o




obj-$(CONFIG_CALL_THUNKS)		+= callthunks.o




###
# 64 bit specific files
ifeq ($(CONFIG_X86_64),y)




	obj-$(CONFIG_MMCONF_FAM10H)	+= mmconf-fam10h_64.o
endif
