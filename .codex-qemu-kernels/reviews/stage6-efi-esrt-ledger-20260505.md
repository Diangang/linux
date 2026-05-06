# Stage 6 EFI ESRT ledger

Target: EFI System Resource Table support

Selected bucket:

- `drivers/firmware/efi/esrt.c`
- `CONFIG_EFI_ESRT`
- Mechanical ESRT config, Makefile, defconfig, table pointer, and init-call
  wiring

Assumptions:

- The fixed contract is x86_64 plus arm64 QEMU boot with generated initramfs,
  NVMe `/dev/nvme0n1`, virtio-SCSI HDD `/dev/sda`, and Minix
  mount/write/read/sync/unmount.
- ESRT exports firmware-update metadata under `/sys/firmware/efi/esrt`.
- Firmware update resource discovery is outside the fixed storage contract.
- Core EFI runtime, EFI memory map, EFI pstore, DMI, PSCI/SMCCC, and SCMI
  paths remain out of scope for this patch.

Evidence:

- `.codex-qemu-kernels/build-x86_64/.config` and
  `.codex-qemu-kernels/build-arm64/.config` both have `CONFIG_EFI_ESRT=y`.
- Both fixed builds produce `drivers/firmware/efi/esrt.o`.
- `drivers/firmware/efi/esrt.c` documents sysfs export of EFI System Resource
  Table entries for firmware updates through UEFI capsule update support.
- The fixed QEMU storage validation uses generated initramfs, NVMe, virtio-SCSI
  HDD, and Minix; it does not consume ESRT sysfs state.

Classification:

- `delete_plus_build_wiring`

Deferred:

- Generic EFI runtime, EFI config table parsing, and memory attribute support
  remain because they are broader EFI/boot paths and not part of this ESRT
  removal.
- EFI capsule header/runtime service types remain because runtime wrappers
  still use the generic firmware service signatures.

Verification plan:

- `rg -n "EFI_ESRT|EFI_SYSTEM_RESOURCE_TABLE_GUID|efi_esrt_init|\\.esrt|&efi\\.esrt|esrt\\.o|/sys/firmware/efi/esrt" arch drivers include`
- `git diff --check`
- `find drivers/firmware/efi -type d -empty -print`
- `JOBS=32 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh`
- `git show --stat --oneline --decorate HEAD`
- `git show --check --oneline HEAD`

Validation:

- `JOBS=32 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh`
- Result: passed for x86_64 and arm64.
- x86_64 QEMU log:
  `.codex-qemu-kernels/logs/qemu/minix-storage-x86_64-20260505-220055-attempt1.log`
- arm64 QEMU log:
  `.codex-qemu-kernels/logs/qemu/minix-storage-arm64-20260505-220058-attempt1.log`
- Metrics:
  `.codex-qemu-kernels/metrics/metrics-20260505-220059.txt`

Commit:

- `d8375d4d1b1d refactor(裁剪firmware): 删除EFI ESRT支持`
- Patch class: `delete_plus_build_wiring`

Review:

- Residual ESRT symbol/build reference scan across `arch`, `drivers`, and
  `include`: clean.
- `git diff --check`: clean before commit.
- Empty directory check under `drivers/firmware/efi`: clean.
- `git show --stat --oneline --decorate HEAD`: 10 files changed, 1
  insertion, 451 deletions.
- `git show --check --oneline HEAD`: clean.
- `git status --short`: clean after commit.
