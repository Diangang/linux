# Stage 6 EFI embedded firmware ledger

Target: EFI embedded firmware fallback support

Selected bucket:

- `drivers/firmware/efi/embedded-firmware.c`
- `drivers/base/firmware_loader/fallback_platform.c`
- `include/linux/efi_embedded_fw.h`
- Mechanical Kconfig, Makefile, and inline-stub cleanup for
  `CONFIG_EFI_EMBEDDED_FIRMWARE`

Assumptions:

- The fixed contract is x86_64 plus arm64 QEMU boot with generated initramfs,
  NVMe `/dev/nvme0n1`, virtio-SCSI HDD `/dev/sda`, and Minix
  mount/write/read/sync/unmount.
- EFI embedded firmware scanning and firmware-loader platform fallback are
  optional platform firmware payload features, not required for the fixed QEMU
  storage contract.
- `CONFIG_EFI_EMBEDDED_FIRMWARE` is not enabled in either fixed build and has
  no active fixed-build object dependency.

Evidence:

- No `CONFIG_EFI_EMBEDDED_FIRMWARE` entry is present in the fixed x86_64 or
  arm64 `.config` files.
- No fixed build artifact exists for `embedded-firmware.o` or
  `fallback_platform.o`.
- Kconfig/Makefile references are limited to
  `drivers/firmware/efi/Kconfig`, `drivers/firmware/efi/Makefile`, and
  `drivers/base/firmware_loader/Makefile`.
- Source references are limited to the deleted source/header files and
  `CONFIG_EFI_EMBEDDED_FIRMWARE` guarded stubs in
  `drivers/base/firmware_loader/fallback.h` and `include/linux/efi.h`.

Classification:

- `delete_plus_build_wiring`

Deferred:

- Broader firmware loader and EFI runtime paths remain untouched because they
  are shared by built x86_64 and arm64 code.

Verification plan:

- `rg -n "EFI_EMBEDDED_FIRMWARE|efi_embedded_fw|efi_check_for_embedded_firmwares|fallback_platform" drivers include arch`
- `git diff --check`
- `find drivers/firmware/efi drivers/base/firmware_loader include/linux -type d -empty -print`
- `JOBS=32 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh`
- `git show --stat --oneline --decorate HEAD`
- `git show --check --oneline HEAD`

Validation result:

- Deleted config/header/object references are gone; only the permanent inline
  stubs and existing callers remain.
- `git diff --check`: clean.
- `find drivers/firmware/efi drivers/base/firmware_loader include/linux -type d -empty -print`: no empty directories.
- `JOBS=32 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh`: passed.
- x86_64 QEMU log:
  `.codex-qemu-kernels/logs/qemu/minix-storage-x86_64-20260505-214911-attempt1.log`
- arm64 QEMU log:
  `.codex-qemu-kernels/logs/qemu/minix-storage-arm64-20260505-214914-attempt1.log`
- Metrics: `.codex-qemu-kernels/metrics/metrics-20260505-214915.txt`

Commit:

- `d911d4927e5f refactor(裁剪firmware): 删除EFI embedded firmware fallback`

Review:

- `git show --stat --oneline --decorate HEAD`: 8 files changed, 247 deletions.
- `git show --check --oneline HEAD`: clean.
- No `.codex-qemu-kernels/` files committed.
