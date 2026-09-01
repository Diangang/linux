# SPDX-License-Identifier: GPL-2.0
# ===========================================================================
# Kernel configuration targets
# These targets are used from top-level makefile

ifdef KBUILD_KCONFIG
Kconfig := $(KBUILD_KCONFIG)
else
Kconfig := Kconfig
endif

ifndef KBUILD_DEFCONFIG
KBUILD_DEFCONFIG := defconfig
endif

ifeq ($(quiet),silent_)
silent := -s
endif

export KCONFIG_DEFCONFIG_LIST :=
ifndef cross_compiling
kernel-release := $(shell uname -r)
KCONFIG_DEFCONFIG_LIST += \
	/lib/modules/$(kernel-release)/.config \
	/etc/kernel-config \
	/boot/config-$(kernel-release)
endif
KCONFIG_DEFCONFIG_LIST += arch/$(SRCARCH)/configs/$(KBUILD_DEFCONFIG)

ifneq ($(findstring c, $(KBUILD_EXTRA_WARN)),)
export KCONFIG_WARN_UNKNOWN_SYMBOLS=1
endif

ifneq ($(findstring e, $(KBUILD_EXTRA_WARN)),)
export KCONFIG_WERROR=1
endif

# We need this, in case the user has it in its environment
unexport CONFIG_

config-prog	:= conf

PHONY += config build_config
config: $(obj)/$(config-prog)
	$(Q)$< $(silent) $(Kconfig)

build_config: $(obj)/$(config-prog)

# These targets map 1:1 to the commandline options of 'conf'
#
# Note:
#  syncconfig has become an internal implementation detail and is now
#  deprecated for external use
simple-targets := oldconfig allnoconfig allyesconfig allmodconfig \
	alldefconfig randconfig listnewconfig olddefconfig syncconfig \
	helpnewconfig yes2modconfig mod2yesconfig mod2noconfig

PHONY += $(simple-targets)

$(simple-targets): $(obj)/conf
	$(Q)$< $(silent) --$@ $(Kconfig)

PHONY += savedefconfig defconfig

savedefconfig: $(obj)/conf
	$(Q)$< $(silent) --$@=defconfig $(Kconfig)

defconfig: $(obj)/conf
ifneq ($(wildcard $(srctree)/arch/$(SRCARCH)/configs/$(KBUILD_DEFCONFIG)),)
	@$(kecho) "*** Default configuration is based on '$(KBUILD_DEFCONFIG)'"
	$(Q)$< $(silent) --defconfig=arch/$(SRCARCH)/configs/$(KBUILD_DEFCONFIG) $(Kconfig)
else
	@$(kecho) "*** Default configuration is based on target '$(KBUILD_DEFCONFIG)'"
	$(Q)$(MAKE) -f $(srctree)/Makefile $(KBUILD_DEFCONFIG)
endif

%_defconfig: $(obj)/conf
	$(Q)$< $(silent) --defconfig=arch/$(SRCARCH)/configs/$@ $(Kconfig)

# Help text used by make help
help:
	@echo  'Configuration targets:'
	@echo  '  config	  - Update current config utilising a line-oriented program'
	@echo  '  oldconfig	  - Update current config utilising a provided .config as base'
	@echo  '  defconfig	  - New config with default from ARCH supplied defconfig'
	@echo  '  savedefconfig   - Save current config as ./defconfig (minimal config)'
	@echo  '  allnoconfig	  - New config where all options are answered with no'
	@echo  '  allyesconfig	  - New config where all options are accepted with yes'
	@echo  '  allmodconfig	  - New config selecting modules when possible'
	@echo  '  alldefconfig    - New config with all symbols set to default'
	@echo  '  randconfig	  - New config with random answer to all options'
	@echo  '  yes2modconfig	  - Change answers from yes to mod if possible'
	@echo  '  mod2yesconfig	  - Change answers from mod to yes if possible'
	@echo  '  mod2noconfig	  - Change answers from mod to no if possible'
	@echo  '  listnewconfig   - List new options'
	@echo  '  helpnewconfig   - List new options and help text'
	@echo  '  olddefconfig	  - Same as oldconfig but sets new symbols to their'
	@echo  '                    default value without prompting'

# ===========================================================================
# object files used by all kconfig flavours
common-objs	:= confdata.o expr.o lexer.lex.o menu.o parser.tab.o \
		   preprocess.o symbol.o util.o

$(obj)/lexer.lex.o: $(obj)/parser.tab.h
HOSTCFLAGS_lexer.lex.o	:= -I $(src)
HOSTCFLAGS_parser.tab.o	:= -I $(src)

# conf: Used for defconfig, oldconfig and related targets
hostprogs	+= conf
conf-objs	:= conf.o $(common-objs)
