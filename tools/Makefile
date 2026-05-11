# SPDX-License-Identifier: GPL-2.0
export srctree=
export objtree=

include scripts/Makefile.include

help:
	@echo 'Possible targets:'
	@echo ''
	@echo '  objtool                - an ELF object analysis tool'
	@echo ''
	@echo 'Cleaning targets:'
	@echo ''
	@echo '  clean                  - clean retained kernel build tools'

objtool: FORCE
	$(call descend,$@)

libsubcmd_clean:
	$(call descend,lib/subcmd,clean)

build_clean:
	$(call descend,build,clean)

objtool_clean:
	$(call descend,objtool,clean)

clean: objtool_clean build_clean libsubcmd_clean

.PHONY: FORCE
