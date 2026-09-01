# SPDX-License-Identifier: GPL-2.0
out := arch/$(SRCARCH)/include/generated/asm
uapi := arch/$(SRCARCH)/include/generated/uapi/asm

# Create output directory if not already present
$(shell mkdir -p $(out) $(uapi))

syscall64 := $(src)/syscall_64.tbl

syshdr := $(srctree)/scripts/syscallhdr.sh
systbl := $(srctree)/scripts/syscalltbl.sh
offset :=
prefix :=

quiet_cmd_syshdr = SYSHDR  $@
      cmd_syshdr = $(CONFIG_SHELL) $(syshdr) --abis $(abis) --emit-nr \
		$(if $(offset),--offset $(offset)) \
		$(if $(prefix),--prefix $(prefix)) \
		$< $@
quiet_cmd_systbl = SYSTBL  $@
      cmd_systbl = $(CONFIG_SHELL) $(systbl) --abis $(abis) $< $@

$(uapi)/unistd_x32.h: abis := common,x32
$(uapi)/unistd_x32.h: offset := __X32_SYSCALL_BIT
$(uapi)/unistd_x32.h: $(syscall64) $(syshdr) FORCE
	$(call if_changed,syshdr)

$(uapi)/unistd_64.h: abis := common,64
$(uapi)/unistd_64.h: $(syscall64) $(syshdr) FORCE
	$(call if_changed,syshdr)

$(out)/unistd_64_x32.h: abis := x32
$(out)/unistd_64_x32.h: prefix := x32_
$(out)/unistd_64_x32.h: $(syscall64) $(syshdr) FORCE
	$(call if_changed,syshdr)

$(out)/syscalls_64.h: abis := common,64
$(out)/syscalls_64.h: $(syscall64) $(systbl) FORCE
	$(call if_changed,systbl)
$(out)/syscalls_x32.h: abis := common,x32
$(out)/syscalls_x32.h: $(syscall64) $(systbl) FORCE
	$(call if_changed,systbl)

uapisyshdr-y			+= unistd_64.h unistd_x32.h
syshdr-$(CONFIG_X86_64)		+= unistd_64_x32.h
syshdr-$(CONFIG_X86_64)		+= syscalls_64.h

uapisyshdr-y	:= $(addprefix $(uapi)/, $(uapisyshdr-y))
syshdr-y	:= $(addprefix $(out)/, $(syshdr-y))
targets		+= $(addprefix ../../../../, $(uapisyshdr-y) $(syshdr-y))

PHONY += all
all: $(uapisyshdr-y) $(syshdr-y)
	@:
