# Stage 6 firmware ledger: EFI TPM event log

## Target

- Deletion unit: EFI TPM/CC event-log retrieval and reservation
- Source files: `drivers/firmware/efi/tpm.c`, `drivers/firmware/efi/libstub/tpm.c`
- Patch class: `delete_plus_build_wiring`
- Stage: whole-file straggler under `drivers/firmware/efi/`

## Assumptions

- The fixed validation contract requires generated-initramfs boot and Minix I/O on QEMU NVMe `/dev/nvme0n1` plus SCSI `/dev/sda`.
- QEMU validation does not provide a TPM/CC EFI event-log dependency; recent arm64 logs report `efi: UEFI not found.`
- Secureboot helpers and generic EFI measured-event logic remain; this patch removes only copying/reserving EFI TPM/CC event logs for Linux.
- Removing event-log table plumbing is outside storage behavior and does not alter NVMe, SCSI, block, VFS, Minix, or device names.

## Dependency scan

- Build wiring:
  - `drivers/firmware/efi/Makefile` always builds `tpm.o` under `CONFIG_EFI`.
  - `drivers/firmware/efi/libstub/Makefile` always includes `tpm.o` in `lib-y`.
- Direct source users:
  - `drivers/firmware/efi/efi.c` initializes `efi.tpm_log` and `efi.tpm_final_log`, records TPM/CC event-log config tables, and calls `efi_tpm_eventlog_init()`.
  - `drivers/firmware/efi/libstub/efi-stub.c` and `drivers/firmware/efi/libstub/x86-stub.c` call `efi_retrieve_eventlog()`.
  - `include/linux/efi.h` declares EFI TPM log fields, structures, and `efi_tpm_eventlog_init()`.
  - `drivers/firmware/efi/libstub/efistub.h` declares `efi_retrieve_eventlog()`.
  - `arch/x86/platform/efi/efi.c` lists `efi.tpm_log` and `efi.tpm_final_log` in EFI table address handling.
- Runtime evidence:
  - Fixed x86_64 and arm64 QEMU Minix validation passes with serial console, PCI NVMe, virtio-scsi/SCSI disk, and no observed TPM event-log dependency.

## Planned edit

- Delete `drivers/firmware/efi/tpm.c` and `drivers/firmware/efi/libstub/tpm.c`.
- Remove their Makefile object entries.
- Remove direct EFI TPM event-log fields, table entries, declarations, and calls.
- Leave secureboot, reset attack mitigation, EFI random seed, and generic TCG2 measured-event helper code intact.

## Verification plan

1. Scan for residual `efi_tpm_eventlog_init`, `efi_retrieve_eventlog`, `efi_tpm_final_log_size`, `tpm_log`, `tpm_final_log`, and deleted TPM object references.
2. Run `git diff --check`.
3. Check `drivers/firmware/efi` and `drivers/firmware/efi/libstub` have no empty directory leftovers.
4. Run `JOBS=32 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh`.

## Verification result

- Residual scan: clean; no remaining EFI TPM event-log init/retrieval fields, table names, or deleted object references in `arch`, `drivers`, `include`, or generated x86_64/arm64 configs.
- `git diff --check`: clean.
- Empty directory check: clean for `drivers/firmware/efi` and `drivers/firmware/efi/libstub`.
- Fixed storage validation: passed with `JOBS=32 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh`.
- x86_64 log: `.codex-qemu-kernels/logs/qemu/minix-storage-x86_64-20260505-223849-attempt1.log`.
- arm64 log: `.codex-qemu-kernels/logs/qemu/minix-storage-arm64-20260505-223852-attempt1.log`.
- Metrics: `.codex-qemu-kernels/metrics/metrics-20260505-223853.txt`.

## Post-commit review

- Commit: `f6b444d66c33 refactor(裁剪firmware): 删除EFI TPM事件日志`.
- `git show --stat --oneline --decorate HEAD`: 10 source files changed, 2 insertions, 308 deletions; deleted `drivers/firmware/efi/tpm.c` and `drivers/firmware/efi/libstub/tpm.c`.
- `git show --check --oneline HEAD`: clean.
- `git show --name-status --oneline HEAD`: source-only EFI TPM event-log deletion and wiring cleanup.
- `git status --short`: clean after source commit.
- Review finding: none.
