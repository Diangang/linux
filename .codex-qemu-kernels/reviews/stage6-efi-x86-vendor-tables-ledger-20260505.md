# Stage 6 EFI x86 vendor tables ledger

Target: `drivers/firmware/efi/`

Selected bucket:

- `drivers/firmware/efi/apple-properties.c`
- `drivers/firmware/efi/dev-path-parser.c`
- `drivers/firmware/efi/rci2-table.c`
- Mechanical Kconfig/Makefile/x86 defconfig references for
  `CONFIG_APPLE_PROPERTIES`, `CONFIG_EFI_DEV_PATH_PARSER`, and
  `CONFIG_EFI_RCI2_TABLE`

Assumptions:

- The fixed contract is x86_64 plus arm64 QEMU boot with generated initramfs,
  NVMe `/dev/nvme0n1`, virtio-SCSI HDD `/dev/sda`, and Minix
  mount/write/read/sync/unmount.
- Apple EFI properties and Dell EMC RCI2 firmware table exposure are x86 vendor
  platform features, not required by the fixed QEMU storage contract.
- `EFI_DEV_PATH_PARSER` is only selected by `APPLE_PROPERTIES` in this tree, so
  it can be removed with the Apple properties file.

Evidence:

- `.codex-qemu-kernels/build-x86_64/.config` has
  `# CONFIG_APPLE_PROPERTIES is not set` and
  `# CONFIG_EFI_RCI2_TABLE is not set`.
- `arch/x86/configs/x86_64_defconfig` has the same two disabled symbols.
- `.codex-qemu-kernels/build-arm64/.config` does not enable either symbol.
- Current x86_64 and arm64 build artifacts under
  `.codex-qemu-kernels/build-*/drivers/firmware/efi/` do not contain
  `apple-properties.o`, `dev-path-parser.o`, or `rci2-table.o`.
- Source references are limited to `drivers/firmware/efi/Kconfig`,
  `drivers/firmware/efi/Makefile`, disabled `IS_ENABLED()`/`#ifdef` branches,
  and the deleted files.

Classification:

- `delete_plus_build_wiring`

Deferred:

- Existing false `IS_ENABLED(CONFIG_APPLE_PROPERTIES)` and
  `#ifdef CONFIG_EFI_RCI2_TABLE` references are not removed in this pass because
  that would be file-internal cleanup outside the selected whole-file deletion
  wiring.

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
  `.codex-qemu-kernels/logs/qemu/minix-storage-x86_64-20260505-213906-attempt1.log`
- arm64 QEMU log:
  `.codex-qemu-kernels/logs/qemu/minix-storage-arm64-20260505-213909-attempt1.log`
- Metrics: `.codex-qemu-kernels/metrics/metrics-20260505-213910.txt`

Commit:

- `1bf8e2455999 refactor(裁剪firmware): 删除EFI x86 vendor tables`

Review:

- `git show --stat --oneline --decorate HEAD`: 6 files changed, 594 deletions.
- `git show --check --oneline HEAD`: clean.
- No `.codex-qemu-kernels/` files committed.
