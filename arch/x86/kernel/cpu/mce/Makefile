# SPDX-License-Identifier: GPL-2.0
obj-y				=  core.o severity.o genpool.o

obj-$(CONFIG_X86_MCE_INTEL)	+= intel.o
obj-$(CONFIG_X86_MCE_AMD)	+= amd.o
obj-$(CONFIG_X86_MCE_THRESHOLD) += threshold.o

mce-inject-y			:= inject.o

obj-$(CONFIG_ACPI_APEI)		+= apei.o
