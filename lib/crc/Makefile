# SPDX-License-Identifier: GPL-2.0-only

# Makefile for the kernel's cyclic redundancy check (CRC) library code

obj-$(CONFIG_CRC32) += crc32.o
crc32-y := crc32-main.o
ifeq ($(CONFIG_CRC32_ARCH),y)
CFLAGS_crc32-main.o += -I$(src)/$(SRCARCH)
crc32-$(CONFIG_ARM64) += arm64/crc32-core.o
crc32-$(CONFIG_X86) += x86/crc32-pclmul.o
crc32-$(CONFIG_X86_64) += x86/crc32c-3way.o
endif

hostprogs := gen_crc32table
clean-files := crc32table.h

$(obj)/crc32-main.o: $(obj)/crc32table.h

quiet_cmd_crc32 = GEN     $@
      cmd_crc32 = $< > $@

$(obj)/crc32table.h: $(obj)/gen_crc32table
	$(call cmd,crc32)
