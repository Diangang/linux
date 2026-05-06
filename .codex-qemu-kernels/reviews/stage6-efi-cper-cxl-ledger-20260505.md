# Stage 6 EFI CPER CXL ledger

Target: EFI CPER CXL protocol-error pretty printer

Selected bucket:

- `drivers/firmware/efi/cper_cxl.c`
- Mechanical CPER Makefile, print dispatch, and prototype wiring for
  `cxl_cper_print_prot_err()`

Assumptions:

- The fixed contract is x86_64 plus arm64 QEMU boot with generated initramfs,
  NVMe `/dev/nvme0n1`, virtio-SCSI HDD `/dev/sda`, and Minix
  mount/write/read/sync/unmount.
- CXL protocol error pretty-printing is not part of the fixed QEMU NVMe or
  virtio-SCSI storage path.
- General CPER remains because arm64 ACPI APEI selects `CONFIG_UEFI_CPER`.
- This patch does not remove GHES, APEI, or generic CPER validation/printing.

Evidence:

- `.codex-qemu-kernels/build-arm64/.config` has `CONFIG_UEFI_CPER=y`, so
  `cper_cxl.o` is currently built through the generic CPER Makefile edge.
- `drivers/firmware/efi/cper_cxl.c` only implements
  `cxl_cper_print_prot_err()`.
- The only direct caller in the firmware tree is the CXL Protocol Error branch
  in `drivers/firmware/efi/cper.c`.
- The fixed QEMU storage validation has no CXL devices and only requires QEMU
  NVMe plus virtio-SCSI HDD block paths.

Classification:

- `delete_plus_build_wiring`

Deferred:

- `drivers/firmware/efi/cper.c`, `drivers/firmware/efi/cper-arm.c`, and the
  generic `CONFIG_UEFI_CPER` Kconfig entries remain because they are selected
  by ACPI APEI and are broader hardware-error reporting support.
- CXL GHES event plumbing outside `drivers/firmware/efi` remains for a later
  non-firmware proof pass, if selected.

Verification plan:

- `rg -n "cper_cxl|cxl_cper_print_prot_err|CPER_SEC_CXL_PROT_ERR|CXL Protocol Error" drivers/firmware include/linux`
- `git diff --check`
- `find drivers/firmware/efi -type d -empty -print`
- `JOBS=32 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh`
- `git show --stat --oneline --decorate HEAD`
- `git show --check --oneline HEAD`

Validation:

- Residual removed-symbol scan:
  `rg -n "cper_cxl|cxl_cper_print_prot_err" drivers/firmware include/linux`
  produced no matches.
- `git diff --check` produced no output.
- `find drivers/firmware/efi -type d -empty -print` produced no output.
- Fixed storage contract passed:
  `JOBS=32 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh`
- x86_64 QEMU log:
  `.codex-qemu-kernels/logs/qemu/minix-storage-x86_64-20260505-220335-attempt1.log`
- arm64 QEMU log:
  `.codex-qemu-kernels/logs/qemu/minix-storage-arm64-20260505-220338-attempt1.log`
- Metrics:
  `.codex-qemu-kernels/metrics/metrics-20260505-220339.txt`

Commit:

- `59c7455b3cb4 refactor(裁剪firmware): 删除EFI CPER CXL打印支持`

Post-commit review:

- `git show --stat --oneline --decorate HEAD` reports only the selected EFI
  CPER CXL pretty-printer source, Makefile edge, dispatch branch, and prototype
  removal: 4 files changed, 1 insertion, 170 deletions.
- `git show --check --oneline HEAD` produced no whitespace or patch-format
  findings.
- `git status --short` was clean for tracked source files.

Review result:

- Clean. Patch remains `delete_plus_build_wiring`; no runtime storage, Minix,
  NVMe, SCSI, mount, read/write, sync, or device-name behavior was changed.
