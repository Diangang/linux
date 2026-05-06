# Stage 6 EFI RISC-V runtime ledger

Target: `drivers/firmware/efi/riscv-runtime.c`

Selected bucket:

- `drivers/firmware/efi/riscv-runtime.c`
- Mechanical `drivers/firmware/efi/Makefile` RISC-V object wiring

Assumptions:

- The fixed contract covers only x86_64 and arm64 QEMU boot with generated
  initramfs, NVMe `/dev/nvme0n1`, virtio-SCSI HDD `/dev/sda`, and Minix
  mount/write/read/sync/unmount.
- RISC-V EFI runtime code is outside the architecture scope and not shared by
  the fixed x86_64 or arm64 storage path.
- Other-architecture breakage is out of scope unless the changed file is shared
  with x86_64 or arm64; this file is RISC-V-specific.

Evidence:

- Fixed x86_64 and arm64 `.config` files do not enable `CONFIG_RISCV`.
- No fixed build artifact exists for `riscv-runtime.o`.
- Source references are limited to `drivers/firmware/efi/Makefile` RISC-V
  object wiring and the RISC-V-specific source file.

Classification:

- `delete_plus_build_wiring`

Deferred:

- Generic EFI runtime files remain untouched because they are built by x86_64
  and/or arm64.
- Larger disabled `SYSFB` cleanup is deferred because its header and callers
  span EFI, PCI, OF, and arch setup paths.

Verification plan:

- `git diff --check`
- `find drivers/firmware/efi -type d -empty -print`
- `JOBS=32 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh`
- `git show --stat --oneline --decorate HEAD`
- `git show --check --oneline HEAD`

Validation result:

- `git diff --check`: clean.
- `find drivers/firmware/efi -type d -empty -print`: no empty directories.
- `JOBS=32 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh`: passed.
- x86_64 QEMU log:
  `.codex-qemu-kernels/logs/qemu/minix-storage-x86_64-20260505-214621-attempt1.log`
- arm64 QEMU log:
  `.codex-qemu-kernels/logs/qemu/minix-storage-arm64-20260505-214624-attempt1.log`
- Metrics: `.codex-qemu-kernels/metrics/metrics-20260505-214625.txt`

Commit:

- `e90cdd7deb57 refactor(裁剪firmware): 删除EFI RISC-V runtime`

Review:

- `git show --stat --oneline --decorate HEAD`: 2 files changed, 161 deletions.
- `git show --check --oneline HEAD`: clean.
- No `.codex-qemu-kernels/` files committed.
