# SPDX-License-Identifier: GPL-2.0
#
# Makefile for x86-compatible CPU details, features and quirks
#

# If these files are instrumented, boot hangs during the first second.

obj-y			:= cacheinfo.o scattered.o
obj-y			+= topology_common.o topology_ext.o topology_amd.o
obj-y			+= common.o
obj-y			+= rdrand.o
obj-y			+= match.o
obj-y			+= bugs.o
obj-y			+= aperfmperf.o
obj-y			+= cpuid-deps.o cpuid_0x2_table.o
obj-y			+= umwait.o
obj-y 			+= capflags.o powerflags.o

obj-$(CONFIG_X86_LOCAL_APIC)		+= topology.o

obj-$(CONFIG_PROC_FS)			+= proc.o

obj-$(CONFIG_IA32_FEAT_CTL)		+= feat_ctl.o
ifdef CONFIG_CPU_SUP_INTEL
obj-y					+= intel.o tsx.o
endif
obj-$(CONFIG_CPU_SUP_AMD)		+= amd.o
ifeq ($(CONFIG_AMD_NB)$(CONFIG_SYSFS),yy)
obj-y					+= amd_cache_disable.o
endif


obj-$(CONFIG_X86_MCE)			+= mce/
obj-$(CONFIG_MTRR)			+= mtrr/



obj-$(CONFIG_X86_BUS_LOCK_DETECT)	+= bus_lock.o

quiet_cmd_mkcapflags = MKCAP   $@
      cmd_mkcapflags = $(CONFIG_SHELL) $(src)/mkcapflags.sh $@ $^

cpufeature = $(src)/../../include/asm/cpufeatures.h
vmxfeature = $(src)/../../include/asm/vmxfeatures.h

$(obj)/capflags.c: $(cpufeature) $(vmxfeature) $(src)/mkcapflags.sh FORCE
	$(call if_changed,mkcapflags)
targets += capflags.c
