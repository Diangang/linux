# Stage 6 EFI capsule loader ledger

Target: EFI capsule loader and capsule update submission support

Selected bucket:

- `drivers/firmware/efi/capsule.c`
- `drivers/firmware/efi/capsule-loader.c`
- `CONFIG_EFI_CAPSULE_LOADER`
- `CONFIG_EFI_CAPSULE_QUIRK_QUARK_CSH`
- Mechanical EFI Makefile and arm64 defconfig wiring

Assumptions:

- The fixed contract is x86_64 plus arm64 QEMU boot with generated initramfs,
  NVMe `/dev/nvme0n1`, virtio-SCSI HDD `/dev/sda`, and Minix
  mount/write/read/sync/unmount.
- Firmware capsule update upload through `/dev/efi_capsule_loader` is outside
  that storage contract.
- x86_64 already has `CONFIG_EFI_CAPSULE_LOADER` disabled.
- arm64 fixed validation uses raw `Image` and does not depend on EFI capsule
  update submission.

Evidence:

- `.codex-qemu-kernels/build-x86_64/.config` has
  `# CONFIG_EFI_CAPSULE_LOADER is not set`.
- `.codex-qemu-kernels/build-arm64/.config` has
  `CONFIG_EFI_CAPSULE_LOADER=y`; the only built capsule objects are
  `drivers/firmware/efi/capsule.o` and
  `drivers/firmware/efi/capsule-loader.o`.
- `arch/arm64/configs/defconfig` directly enables
  `CONFIG_EFI_CAPSULE_LOADER=y`.
- The capsule loader Kconfig help documents a user-facing loader device
  `/dev/efi_capsule_loader`, not a boot storage path.
- Remaining EFI runtime wrappers expose generic firmware runtime service
  function pointers and are kept; deleting them would touch broader EFI
  runtime semantics.

Classification:

- `delete_plus_build_wiring`

Deferred:

- `drivers/firmware/efi/runtime-wrappers.c` remains because it is selected by
  `CONFIG_EFI_RUNTIME_WRAPPERS` and carries generic EFI runtime service
  wrappers beyond capsule upload.
- Generic EFI capsule header/runtime service types in `include/linux/efi.h`
  remain because EFI runtime wrappers still use them. Loader-only
  `struct capsule_info`, setup helpers, and exported update helpers were
  removed as direct fallout from deleting the loader.
- `drivers/firmware/efi/reboot.c` remains unchanged; with capsule loader
  removed, the existing `efi_capsule_pending()` declaration is now the
  always-false inline path.

Verification plan:

- `rg -n "EFI_CAPSULE_LOADER|EFI_CAPSULE_QUIRK_QUARK_CSH|efi_capsule_loader|obj-\\$\\(CONFIG_EFI_CAPSULE_LOADER\\)|capsule-loader\\.o|capsule\\.o" arch drivers include`
- `git diff --check`
- `find drivers/firmware/efi -type d -empty -print`
- `JOBS=32 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh`
- `git show --stat --oneline --decorate HEAD`
- `git show --check --oneline HEAD`

Validation:

- `JOBS=32 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh`
- Result: passed for x86_64 and arm64.
- x86_64 QEMU log:
  `.codex-qemu-kernels/logs/qemu/minix-storage-x86_64-20260505-215717-attempt1.log`
- arm64 QEMU log:
  `.codex-qemu-kernels/logs/qemu/minix-storage-arm64-20260505-215720-attempt1.log`
- Metrics:
  `.codex-qemu-kernels/metrics/metrics-20260505-215722.txt`

Commit:

- `8848c86aae3c refactor(裁剪firmware): 删除EFI capsule loader`
- Patch class: `delete_plus_build_wiring`

Review:

- Residual capsule loader symbol/build reference scan across `arch`,
  `drivers`, and `include`: clean.
- `git diff --check`: clean before commit.
- Empty directory check under `drivers/firmware/efi`: clean.
- `git show --stat --oneline --decorate HEAD`: 8 files changed, 798
  deletions.
- `git show --check --oneline HEAD`: clean.
- `git status --short`: clean after commit.
