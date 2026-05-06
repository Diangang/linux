# Stage 6 firmware ledger: EFI ARM CPER printer

## Target

- Deletion unit: `drivers/firmware/efi/cper-arm.c`
- Patch class: `delete_plus_build_wiring`
- Stage: whole-file straggler under `drivers/firmware/efi/`

## Assumptions

- The fixed validation contract is limited to x86_64 and arm64 QEMU boot plus NVMe/SCSI Minix mount/write/read/sync/unmount.
- ARM CPER processor-error pretty-printing is firmware error-report decoding, not a storage or boot dependency for the fixed QEMU path.
- Generic CPER core, CPER data structures, and APEI/GHES paths remain; this patch only removes the ARM-specific print formatter object and its direct call site.
- Recent arm64 QEMU validation reports `efi: UEFI not found.` and contains no CPER/GHES error report path.

## Dependency scan

- Build wiring:
  - `drivers/firmware/efi/Makefile` builds `cper-arm.o` from `CONFIG_UEFI_CPER_ARM`.
  - `drivers/firmware/efi/Kconfig` defines `UEFI_CPER_ARM`.
  - `arch/arm64/configs/defconfig` requests `CONFIG_UEFI_CPER_ARM=y`.
- Direct source users:
  - `drivers/firmware/efi/cper.c` calls `cper_print_proc_arm()` only in the ARM processor-error section printer.
  - `include/linux/cper.h` declares `cper_print_proc_arm()`.
  - `drivers/acpi/apei/ghes.c` uses ARM CPER data structures and generic string helpers, but does not call `cper_print_proc_arm()`.
- Runtime evidence:
  - The fixed QEMU validation exercises serial console, PCI/NVMe, virtio-scsi/SCSI disk, Minix, and sync/umount. It does not depend on firmware CPER processor-error pretty printing.

## Planned edit

- Delete `drivers/firmware/efi/cper-arm.c`.
- Remove `UEFI_CPER_ARM` Kconfig entry.
- Remove `cper-arm.o` Makefile wiring.
- Remove `CONFIG_UEFI_CPER_ARM=y` from arm64 defconfig.
- Remove the now-dead `cper_print_proc_arm()` declaration and direct ARM CPER print branch in `cper.c`.

## Verification plan

1. Scan for residual `CONFIG_UEFI_CPER_ARM`, `UEFI_CPER_ARM`, `cper_print_proc_arm`, and `cper-arm.o` references.
2. Run `git diff --check`.
3. Check `drivers/firmware/efi` has no empty directory leftovers.
4. Run `JOBS=32 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh`.

## Verification result

- Residual scan after validation found no `CONFIG_UEFI_CPER_ARM`, `UEFI_CPER_ARM`, `cper_print_proc_arm`, `cper-arm.o`, or `cper-arm` references in source or regenerated fixed configs.
- `git diff --check` passed.
- `find drivers/firmware/efi -type d -empty -print` produced no empty directories.
- Fixed storage validation passed with generated initramfs and `rdinit=/init`:
  - x86_64 QEMU log: `.codex-qemu-kernels/logs/qemu/minix-storage-x86_64-20260505-223525-attempt1.log`
  - arm64 QEMU log: `.codex-qemu-kernels/logs/qemu/minix-storage-arm64-20260505-223528-attempt1.log`
  - metrics: `.codex-qemu-kernels/metrics/metrics-20260505-223529.txt`

## Post-commit review

- Commit: `37cb377a9332 refactor(裁剪firmware): 删除ARM CPER打印器`
- `git show --stat --oneline --decorate HEAD` matched the intended 6-file source patch: one deleted ARM CPER printer plus direct config, build, declaration, and call-site cleanup.
- `git show --check --oneline HEAD` reported no whitespace errors.
- `git show --name-status --oneline HEAD` showed no unrelated subsystem changes.
- `git status --short` was clean for kernel source after the commit.
