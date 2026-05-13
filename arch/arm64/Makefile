#
# arch/arm64/Makefile
#
# This file is included by the global makefile so that you can add your own
# architecture-specific flags and dependencies.
#
# This file is subject to the terms and conditions of the GNU General Public
# License.  See the file "COPYING" in the main directory of this archive
# for more details.
#
# Copyright (C) 1995-2001 by Russell King

LDFLAGS_vmlinux	:=--no-undefined -X --pic-veneer

ifeq ($(CONFIG_RELOCATABLE), y)
# Pass --no-apply-dynamic-relocs to restore pre-binutils-2.27 behaviour
# for relative relocs, since this leads to better Image compression
# with the relocation offsets always being zero.
LDFLAGS_vmlinux	+= -shared -Bsymbolic -z notext --no-apply-dynamic-relocs
endif

ifeq ($(CONFIG_ARM64_ERRATUM_843419),y)
LDFLAGS_vmlinux	+= --fix-cortex-a53-843419
endif

cc_has_k_constraint := $(call try-run,echo				\
	'int main(void) {						\
		asm volatile("and w0, w0, %w0" :: "K" (4294967295));	\
		return 0;						\
	}' | $(CC) -S -x c -o "$$TMP" -,,-DCONFIG_CC_HAS_K_CONSTRAINT=1)

# The GCC option -ffreestanding is required in order to compile code containing
# ARM/NEON intrinsics in a non C99-compliant environment (such as the kernel)
CC_FLAGS_FPU	:= -ffreestanding
# Enable <arm_neon.h>
CC_FLAGS_FPU	+= -isystem $(shell $(CC) -print-file-name=include)
CC_FLAGS_NO_FPU	:= -mgeneral-regs-only

KBUILD_CFLAGS	+= $(CC_FLAGS_NO_FPU) \
		   $(compat_vdso) $(cc_has_k_constraint)
KBUILD_CFLAGS	+= $(call cc-disable-warning, psabi)
KBUILD_AFLAGS	+= $(compat_vdso)

ifeq ($(call rustc-min-version, 108500),y)
KBUILD_RUSTFLAGS += --target=aarch64-unknown-none-softfloat
else
KBUILD_RUSTFLAGS += --target=aarch64-unknown-none -Ctarget-feature="-neon"
endif

KBUILD_CFLAGS	+= $(call cc-option,-mabi=lp64)
KBUILD_AFLAGS	+= $(call cc-option,-mabi=lp64)

# Avoid generating .eh_frame* sections.
ifneq ($(CONFIG_UNWIND_TABLES),y)
KBUILD_CFLAGS	+= -fno-asynchronous-unwind-tables -fno-unwind-tables
KBUILD_AFLAGS	+= -fno-asynchronous-unwind-tables -fno-unwind-tables
KBUILD_RUSTFLAGS += -Cforce-unwind-tables=n
else
KBUILD_CFLAGS	+= -fasynchronous-unwind-tables
KBUILD_AFLAGS	+= -fasynchronous-unwind-tables
KBUILD_RUSTFLAGS += -Cforce-unwind-tables=y -Zuse-sync-unwind=n
endif

ifeq ($(CONFIG_STACKPROTECTOR_PER_TASK),y)
prepare: stack_protector_prepare
stack_protector_prepare: prepare0
	$(eval KBUILD_CFLAGS += -mstack-protector-guard=sysreg		  \
				-mstack-protector-guard-reg=sp_el0	  \
				-mstack-protector-guard-offset=$(shell	  \
			awk '{if ($$2 == "TSK_STACK_CANARY") print $$3;}' \
				$(objtree)/include/generated/asm-offsets.h))
endif

ifeq ($(CONFIG_ARM64_BTI_KERNEL),y)
  KBUILD_CFLAGS += -mbranch-protection=pac-ret+bti
  KBUILD_RUSTFLAGS += -Zbranch-protection=bti,pac-ret
else ifeq ($(CONFIG_ARM64_PTR_AUTH_KERNEL),y)
  KBUILD_RUSTFLAGS += -Zbranch-protection=pac-ret
  KBUILD_CFLAGS += -msign-return-address=non-leaf
else
  KBUILD_CFLAGS += $(call cc-option,-mbranch-protection=none)
endif

# Tell the assembler to support instructions from the latest target
# architecture.
#
# For non-integrated assemblers we'll pass this on the command line, and for
# integrated assemblers we'll define ARM64_ASM_ARCH and ARM64_ASM_PREAMBLE for
# inline usage.
#
# We cannot pass the same arch flag to the compiler as this would allow it to
# freely generate instructions which are not supported by earlier architecture
# versions, which would prevent a single kernel image from working on earlier
# hardware.
asm-arch := armv8.4-a

ifdef asm-arch
KBUILD_CFLAGS	+= -Wa,-march=$(asm-arch) \
		   -DARM64_ASM_ARCH='"$(asm-arch)"'
endif

ifeq ($(CONFIG_SHADOW_CALL_STACK), y)
KBUILD_CFLAGS	+= -ffixed-x18
KBUILD_RUSTFLAGS += -Zfixed-x18
endif

KBUILD_CPPFLAGS	+= -mlittle-endian
CHECKFLAGS	+= -D__AARCH64EL__
# Same as above, prefer ELF but fall back to linux target if needed.
KBUILD_LDFLAGS	+= -EL $(call ld-option, -maarch64elf, -maarch64linux -z norelro)
UTS_MACHINE	:= aarch64

ifeq ($(CONFIG_LD_IS_LLD), y)
KBUILD_LDFLAGS	+= -z norelro
endif

CHECKFLAGS	+= -D__aarch64__

ifeq ($(CONFIG_KASAN_SW_TAGS), y)
KASAN_SHADOW_SCALE_SHIFT := 4
else ifeq ($(CONFIG_KASAN_GENERIC), y)
KASAN_SHADOW_SCALE_SHIFT := 3
endif

KBUILD_CFLAGS += -DKASAN_SHADOW_SCALE_SHIFT=$(KASAN_SHADOW_SCALE_SHIFT)
KBUILD_CPPFLAGS += -DKASAN_SHADOW_SCALE_SHIFT=$(KASAN_SHADOW_SCALE_SHIFT)
KBUILD_AFLAGS += -DKASAN_SHADOW_SCALE_SHIFT=$(KASAN_SHADOW_SCALE_SHIFT)

libs-y		:= arch/arm64/lib/ $(libs-y)
libs-$(CONFIG_EFI_STUB) += $(objtree)/drivers/firmware/efi/libstub/lib.a

# Default target when executing plain make
boot		:= arch/arm64/boot

BOOT_TARGETS	:= Image

PHONY += $(BOOT_TARGETS)

KBUILD_IMAGE	:= $(boot)/Image.gz

all:	$(notdir $(KBUILD_IMAGE))

$(BOOT_TARGETS): vmlinux
	$(Q)$(MAKE) $(build)=$(boot) $(boot)/$@

Image.%: Image
	$(Q)$(MAKE) $(build)=$(boot) $(boot)/$@

archprepare:
	$(Q)$(MAKE) $(build)=arch/arm64/tools kapi

ifeq ($(KBUILD_EXTMOD),)
# We need to generate vdso-offsets.h before compiling certain files in kernel/.
# In order to do that, we should use the archprepare target, but we can't since
# asm-offsets.h is included in some files used to generate vdso-offsets.h, and
# asm-offsets.h is built in prepare0, for which archprepare is a dependency.
# Therefore we need to generate the header after prepare0 has been made, hence
# this hack.
prepare: vdso_prepare
vdso_prepare: prepare0
	$(Q)$(MAKE) $(build)=arch/arm64/kernel/vdso \
	include/generated/vdso-offsets.h arch/arm64/kernel/vdso/vdso.so
endif

vdso-install-y				+= arch/arm64/kernel/vdso/vdso.so.dbg

include $(srctree)/scripts/Makefile.defconf

PHONY += virtconfig
virtconfig:
	$(call merge_into_defconfig_override,defconfig,virt)

define archhelp
  echo  '* Image.gz      - Compressed kernel image (arch/$(ARCH)/boot/Image.gz)'
  echo  '  Image         - Uncompressed kernel image (arch/$(ARCH)/boot/Image)'
endef
