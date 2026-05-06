# Stage 6 firmware EFI libstub non-target architecture ledger

Target: `drivers/firmware/efi/libstub/`

Assumptions:
- Fixed validation scope is x86_64 plus arm64 QEMU boot with NVMe
  `/dev/nvme0n1`, SCSI HDD `/dev/sda`, and Minix
  mount/write/read/sync/umount.
- x86_64 and arm64 EFI stub files are kept.
- Other architecture EFI stub files are outside the validation scope and do
  not need to remain buildable for this task.

Evidence:
- Fixed build `.cmd` inventory builds x86_64 and arm64 EFI libstub objects,
  including `x86-stub.o`, `x86-5lvl.o`, `arm64.o`, and `arm64-stub.o`.
- Fixed build `.cmd` inventory does not build ARM32, RISC-V, or LoongArch
  EFI libstub source files.
- `drivers/firmware/efi/libstub/Makefile` selects these files only through
  `CONFIG_ARM`, `CONFIG_RISCV`, or `CONFIG_LOONGARCH`, none of which is a
  fixed validation architecture.

Delete bucket:
- `drivers/firmware/efi/libstub/arm32-stub.c`
- `drivers/firmware/efi/libstub/riscv.c`
- `drivers/firmware/efi/libstub/riscv-stub.c`
- `drivers/firmware/efi/libstub/loongarch.c`
- `drivers/firmware/efi/libstub/loongarch-stub.c`
- `drivers/firmware/efi/libstub/loongarch-stub.h`

Keep/defer:
- Keep common EFI libstub helpers and all x86_64/arm64 selected stub files.
- Defer SCMI optional transports/debug files, PSCI checker, and optional EFI
  runtime leaves to separate proof buckets because they are different firmware
  feature families.
- Defer EFI zboot because arm64 boot Makefile includes the zboot make rules
  even when `CONFIG_EFI_ZBOOT` is disabled.

Patch class:
- `delete_plus_build_wiring`

Planned verification:
- `find drivers/firmware -type d -empty -print | sort`
- `git diff --check -- ':!/.codex-qemu-kernels'`
- `JOBS=32 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh`
- `git show --stat --oneline --decorate HEAD`
- `git show --check --oneline HEAD`

Pre-commit verification:
- Empty source directory check was clean.
- `git diff --check -- ':!/.codex-qemu-kernels'` was clean.
- Fixed storage validation passed with:
  - x86_64 log:
    `.codex-qemu-kernels/logs/qemu/minix-storage-x86_64-20260505-212227-attempt1.log`
  - arm64 log:
    `.codex-qemu-kernels/logs/qemu/minix-storage-arm64-20260505-212230-attempt1.log`
  - metrics:
    `.codex-qemu-kernels/metrics/metrics-20260505-212231.txt`
- Both QEMU guests reached `CODEX_MINIX_TEST_PASS`.

Post-commit review:
- Commit: `f19e961c1978 refactor(裁剪firmware): 删除非目标架构EFI stub`
- `git show --stat --oneline --decorate HEAD` showed 7 source files changed
  with 451 deletions.
- `git show --check --oneline HEAD` was clean.
- `git status --porcelain=v1 -uall -- ':!/.codex-qemu-kernels'` was clean.
- Empty source directory check was clean after commit.
- Review result: clean.
