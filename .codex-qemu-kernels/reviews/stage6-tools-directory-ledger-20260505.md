# Stage 6 tools directory dependency ledger

Target: `tools/`

Assumptions:
- Fixed validation scope is x86_64 plus arm64 QEMU boot with NVMe `/dev/nvme0n1`,
  SCSI HDD `/dev/sda`, and Minix mount/write/read/sync/umount.
- This pass may delete only a complete tools subdirectory or whole source file
  when the fixed build evidence shows no dependency.
- Do not remove a tool if doing so would leave a still-configurable Kconfig or
  Makefile feature half wired.

Evidence:
- x86_64 fixed build uses `tools/objtool/objtool`; generated `.cmd` files list
  direct objtool dependencies for `vmlinux.o`, `.vmlinux.export.o`, boot startup
  objects, and the objtool object files themselves.
- x86_64 objtool builds through `tools/build/`, `tools/lib/subcmd/`,
  `tools/include/`, and `tools/arch/x86/`.
- `scripts/sorttable.c` and `scripts/elf-parse.c` include headers from
  `tools/include/tools/` and `tools/arch/x86/include/`.
- arm64 fixed build has `CONFIG_BPF=y` and `CONFIG_BPF_SYSCALL=y`; generated
  `.cmd` files show kernel objects including
  `tools/lib/bpf/btf_iter.c`, `tools/lib/bpf/btf_relocate.c`,
  `tools/lib/bpf/relo_core.c`, and `tools/lib/bpf/relo_core.h`.
- `tools/bpf/resolve_btfids/` is not built by the current fixed configs because
  `CONFIG_DEBUG_INFO_BTF` is unset, but it remains tied to top-level
  `RESOLVE_BTFIDS`, `resolve_btfids_clean`, `tools/Makefile`, and
  `scripts/gen-btf.sh` BTF wiring.

Classification:
- Keep: `tools/objtool/`, `tools/build/`, `tools/include/`, `tools/arch/x86/`,
  `tools/lib/subcmd/`, and `tools/scripts/` because they are part of the fixed
  x86_64 host-build path.
- Keep: the `tools/lib/bpf/` files used by arm64 kernel BPF objects.
- Defer: `tools/bpf/resolve_btfids/` and the remaining unused-looking
  `tools/lib/bpf/` user-space libbpf files. Removing them cleanly requires a
  focused BTF/libbpf config-and-build-wiring proof, not a standalone tools
  directory deletion.

Patch class:
- No source patch. No safe whole-directory or whole-file deletion was selected
  in this `tools/` pass.

Review result:
- Clean no-delete pass. Continue with the next queued module directory.
