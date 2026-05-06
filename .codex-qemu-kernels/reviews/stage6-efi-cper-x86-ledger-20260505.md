# Stage 6 EFI CPER x86 ledger

Target: EFI CPER IA32/x64 processor-error pretty printer

Selected bucket:

- `drivers/firmware/efi/cper-x86.c`
- Mechanical CPER Makefile, Kconfig, dispatch, and prototype wiring for
  `cper_print_proc_ia()`

Assumptions:

- The fixed contract is x86_64 plus arm64 QEMU boot with generated initramfs,
  NVMe `/dev/nvme0n1`, virtio-SCSI HDD `/dev/sda`, and Minix
  mount/write/read/sync/unmount.
- IA32/x64 CPER processor-error pretty-printing is not part of the fixed QEMU
  NVMe or virtio-SCSI storage path.
- Generic CPER remains because arm64 ACPI APEI selects `CONFIG_UEFI_CPER`.
- ARM CPER remains because arm64 selects `CONFIG_UEFI_CPER_ARM`.

Evidence:

- `.codex-qemu-kernels/build-x86_64/.config` has
  `# CONFIG_ACPI_APEI is not set`, so `CONFIG_UEFI_CPER` and
  `CONFIG_UEFI_CPER_X86` are not enabled for the fixed x86_64 build.
- `.codex-qemu-kernels/build-x86_64/drivers/firmware/efi/` has no
  `cper-x86.o`.
- `.codex-qemu-kernels/build-arm64/.config` has `CONFIG_UEFI_CPER=y` and
  `CONFIG_UEFI_CPER_ARM=y`, but not `CONFIG_UEFI_CPER_X86`.
- `drivers/firmware/efi/cper-x86.c` only implements
  `cper_print_proc_ia()`.
- The direct firmware caller is the `CONFIG_UEFI_CPER_X86` IA32/X64 processor
  error branch in `drivers/firmware/efi/cper.c`.
- The fixed QEMU storage validation has no IA32/X64 CPER processor-error
  reporting requirement.

Classification:

- `delete_plus_build_wiring`

Deferred:

- Generic CPER section constants and structs remain because GHES can still
  identify CPER section GUIDs.
- x86 APEI helpers outside `drivers/firmware/efi` remain for a later
  architecture or ACPI proof pass.

Verification plan:

- `rg -n "cper-x86|cper_print_proc_ia|CONFIG_UEFI_CPER_X86|UEFI_CPER_X86" drivers/firmware include/linux`
- `git diff --check`
- `find drivers/firmware/efi -type d -empty -print`
- `JOBS=32 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh`
- `git show --stat --oneline --decorate HEAD`
- `git show --check --oneline HEAD`

Validation:

- Residual removed-symbol scan:
  `rg -n "cper-x86|cper_print_proc_ia|CONFIG_UEFI_CPER_X86|UEFI_CPER_X86" drivers/firmware include/linux`
  produced no matches.
- `git diff --check` produced no output.
- `find drivers/firmware/efi -type d -empty -print` produced no output.
- Fixed storage contract passed:
  `JOBS=32 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh`
- x86_64 QEMU log:
  `.codex-qemu-kernels/logs/qemu/minix-storage-x86_64-20260505-220814-attempt1.log`
- arm64 QEMU log:
  `.codex-qemu-kernels/logs/qemu/minix-storage-arm64-20260505-220817-attempt1.log`
- Metrics:
  `.codex-qemu-kernels/metrics/metrics-20260505-220818.txt`

Review before commit:

- Source diff is limited to deleting `drivers/firmware/efi/cper-x86.c` and
  its direct EFI CPER Makefile, Kconfig, dispatch, and prototype wiring.
- Patch classification remains `delete_plus_build_wiring`.

Commit:

- `85b925ca9164 refactor(裁剪firmware): 删除EFI x86 CPER打印支持`

Post-commit review:

- `git show --stat --oneline --decorate HEAD` reports only the selected EFI
  x86 CPER pretty-printer source and its direct wiring: 5 files changed, 379
  deletions.
- `git show --check --oneline HEAD` produced no whitespace or patch-format
  findings.
- `git status --short` was clean for tracked source files.

Review result:

- Clean. Patch remains `delete_plus_build_wiring`; no runtime storage, Minix,
  NVMe, SCSI, mount, read/write, sync, or device-name behavior was changed.
