# SPDX-License-Identifier: GPL-2.0
###
# scripts contains sources for various helper programs used throughout
# the kernel for the build process.

hostprogs-always-$(CONFIG_BUILDTIME_TABLE_SORT)		+= sorttable

sorttable-objs := sorttable.o elf-parse.o

filechk_rust_target = $< < include/config/auto.conf

$(obj)/target.json: scripts/generate_rust_target include/config/auto.conf FORCE
	$(call filechk,rust_target)

hostprogs += generate_rust_target
generate_rust_target-rust := y
rustdoc_test_builder-rust := y

HOSTCFLAGS_elf-parse.o = -I$(srctree)/tools/include
HOSTCFLAGS_sorttable.o = -I$(srctree)/tools/include
HOSTLDLIBS_sorttable = -lpthread

ifdef CONFIG_UNWINDER_ORC
ifeq ($(ARCH),x86_64)
SRCARCH := x86
endif
ifeq ($(ARCH),loongarch)
SRCARCH := loongarch
endif
HOSTCFLAGS_sorttable.o += -I$(srctree)/tools/arch/$(SRCARCH)/include
HOSTCFLAGS_sorttable.o += -DUNWINDER_ORC_ENABLED
endif

# The following programs are only built on demand
hostprogs += unifdef gen_packed_field_checks

# Let clean descend into subdirs
subdir-	+= basic dtc kconfig mod
