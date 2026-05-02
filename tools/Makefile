# SPDX-License-Identifier: GPL-2.0
export srctree=
export objtree=

include scripts/Makefile.include

help:
	@echo 'Possible targets:'
	@echo ''
	@echo '  objtool                - an ELF object analysis tool'
	@echo '  bpf/resolve_btfids    - BTF ID resolver used by kernel builds'
	@echo ''
	@echo 'Cleaning targets:'
	@echo ''
	@echo '  clean                  - clean retained kernel build tools'

objtool: FORCE
	$(call descend,$@)

bpf/%: FORCE
	$(call descend,$@)

libbpf_clean:
	$(call descend,lib/bpf,clean)

libsubcmd_clean:
	$(call descend,lib/subcmd,clean)

build_clean:
	$(call descend,build,clean)

objtool_clean:
	$(call descend,objtool,clean)

resolve_btfids_clean:
	$(call descend,bpf/resolve_btfids,clean)

clean: objtool_clean resolve_btfids_clean build_clean libbpf_clean libsubcmd_clean

.PHONY: FORCE
