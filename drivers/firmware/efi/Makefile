# SPDX-License-Identifier: GPL-2.0
#
# Makefile for linux kernel
#

#
# ARM64 maps efi runtime services in userspace addresses
# which don't have KASAN shadow. So dereference of these addresses
# in efi_call_virt() will cause crash if this code instrumented.
#
KASAN_SANITIZE_runtime-wrappers.o	:= n

obj-$(CONFIG_ACPI_BGRT) 		+= efi-bgrt.o
obj-$(CONFIG_EFI)			+= efi.o vars.o reboot.o memattr.o tpm.o
obj-$(CONFIG_EFI)			+= memmap.o
obj-$(CONFIG_EFI_PARAMS_FROM_FDT)	+= fdtparams.o
obj-$(CONFIG_EFI_VARS_PSTORE)		+= efi-pstore.o
obj-$(CONFIG_UEFI_CPER)			+= cper.o
obj-$(CONFIG_EFI_RUNTIME_WRAPPERS)	+= runtime-wrappers.o
subdir-$(CONFIG_EFI_STUB)		+= libstub
obj-$(CONFIG_SYSFB)			+= sysfb_efi.o

arm-obj-$(CONFIG_EFI)			:= efi-init.o arm-runtime.o
obj-$(CONFIG_ARM)			+= $(arm-obj-y)
obj-$(CONFIG_ARM64)			+= $(arm-obj-y)
obj-$(CONFIG_EFI_EARLYCON)		+= earlycon.o
obj-$(CONFIG_UEFI_CPER_ARM)		+= cper-arm.o
obj-$(CONFIG_UEFI_CPER_X86)		+= cper-x86.o
obj-$(CONFIG_UNACCEPTED_MEMORY)		+= unaccepted_memory.o
