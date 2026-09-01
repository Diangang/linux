# SPDX-License-Identifier: GPL-2.0
#
# Makefile for the linux kernel.
#

# Remove stack protector to avoid triggering unneeded stack canary
# checks due to randomize_kstack_offset.
CFLAGS_REMOVE_syscall.o	 = -fstack-protector -fstack-protector-strong
CFLAGS_syscall.o	+= -fno-stack-protector

# It's not safe to invoke KCOV when portions of the kernel environment aren't
# available or are out-of-sync with HW state. Since `noinstr` doesn't always
# inhibit KCOV instrumentation, disable it for the entire compilation unit.

# Object file lists.
obj-y			:= debug-monitors.o entry.o irq.o fpsimd.o		\
			   entry-common.o entry-fpsimd.o process.o ptrace.o	\
			   setup.o signal.o sys.o stacktrace.o time.o traps.o	\
			   io.o vdso.o hyp-stub.o psci.o cpu_ops.o		\
			   return_address.o cpuinfo.o cpu_errata.o		\
			   cpufeature.o alternative.o cacheinfo.o		\
			   smp.o smp_spin_table.o smccc-call.o		\
			   syscall.o proton-pack.o idle.o patching.o pi/	\
			   rsi.o jump_label.o

obj-$(CONFIG_PCI)			+= pci.o
obj-y					+= vdso-wrap.o

# Force dependency (vdso*-wrap.S includes vdso.so through incbin)
$(obj)/vdso-wrap.o: $(obj)/vdso/vdso.so

obj-y					+= head.o
always-$(KBUILD_BUILTIN)		+= vmlinux.lds


# for cleaning
subdir- += vdso
