# Stage 6 EFI MOK variable table ledger

Target: `drivers/firmware/efi/mokvar-table.c`

Selected bucket:

- `drivers/firmware/efi/mokvar-table.c`
- Mechanical `drivers/firmware/efi/Makefile` object reference for
  `CONFIG_LOAD_UEFI_KEYS`

Assumptions:

- The fixed contract is x86_64 plus arm64 QEMU boot with generated initramfs,
  NVMe `/dev/nvme0n1`, virtio-SCSI HDD `/dev/sda`, and Minix
  mount/write/read/sync/unmount.
- UEFI Machine Owner Key table parsing and sysfs exposure is a certificate/key
  loading feature outside that storage contract.
- `CONFIG_LOAD_UEFI_KEYS` is not enabled in either fixed build; current callers
  use the inline stubs in `include/linux/efi.h`.

Evidence:

- No `CONFIG_LOAD_UEFI_KEYS` entry is present in the fixed x86_64 or arm64
  `.config` files.
- No fixed build artifact exists for `mokvar-table.o`.
- Source references outside the deleted file are limited to the dead Makefile
  object edge and `CONFIG_LOAD_UEFI_KEYS` guarded or inline-stub paths in
  `drivers/firmware/efi/efi.c`, `arch/x86/platform/efi/efi.c`,
  `arch/x86/kernel/setup.c`, `drivers/firmware/efi/efi-init.c`, and
  `include/linux/efi.h`.

Classification:

- `delete_plus_build_wiring`

Deferred:

- Header constants, inline stubs, and guarded false branches are left in place
  because removing them would be file-internal surface trimming beyond this
  whole-file deletion.

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
  `.codex-qemu-kernels/logs/qemu/minix-storage-x86_64-20260505-214332-attempt1.log`
- arm64 QEMU log:
  `.codex-qemu-kernels/logs/qemu/minix-storage-arm64-20260505-214336-attempt1.log`
- Metrics: `.codex-qemu-kernels/metrics/metrics-20260505-214337.txt`

Commit:

- `37883b9a8c96 refactor(裁剪firmware): 删除EFI MOK variable table`

Review:

- `git show --stat --oneline --decorate HEAD`: 2 files changed, 361 deletions.
- `git show --check --oneline HEAD`: clean.
- No `.codex-qemu-kernels/` files committed.
