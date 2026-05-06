# Stage 6 firmware ledger: EFI GOP graphics helper

## Target

- Deletion unit: EFI libstub graphics output protocol helper
- Source file: `drivers/firmware/efi/libstub/gop.c`
- Patch class: `delete_plus_build_wiring`
- Stage: whole-file straggler under `drivers/firmware/efi/libstub/`

## Assumptions

- The fixed validation contract uses generated initramfs boot and serial-console QEMU logs.
- Required storage behavior is Minix I/O on QEMU NVMe `/dev/nvme0n1` and SCSI `/dev/sda`.
- EFI GOP framebuffer discovery, EDID copying, and `efifb:` mode selection are outside that storage contract.
- x86_64 and arm64 validation logs use `console=ttyS0` and `console=ttyAMA0`; they do not require EFI framebuffer handoff.

## Dependency scan

- Build wiring:
  - `drivers/firmware/efi/libstub/Makefile` always includes `gop.o` in `lib-y`.
- Direct source users:
  - `drivers/firmware/efi/libstub/efi-stub.c` calls `efi_setup_graphics()` via `setup_primary_display()`.
  - `drivers/firmware/efi/libstub/x86-stub.c` calls `efi_setup_graphics()` via `setup_graphics()`.
  - `drivers/firmware/efi/libstub/efi-stub-helper.c` parses the `efifb:` option via `efi_parse_option_graphics()`.
  - `drivers/firmware/efi/libstub/efistub.h` declares `efi_parse_option_graphics()` and `efi_setup_graphics()`.
- Runtime evidence:
  - The fixed x86_64 and arm64 QEMU commands use `-display none` and serial log files.
  - Recent arm64 QEMU logs show `efi: UEFI not found.` and still complete NVMe/SCSI Minix validation.

## Planned edit

- Delete `drivers/firmware/efi/libstub/gop.c`.
- Remove `gop.o` from EFI libstub build wiring.
- Remove the direct `efi_setup_graphics()` call paths and `efifb:` parser hook.
- Leave command-line loading, initrd handling, secureboot, random seed, memreserve, PCI setup, and serial console paths intact.

## Verification plan

1. Scan for residual `gop.o`, `efi_setup_graphics`, and `efi_parse_option_graphics` references.
2. Run `git diff --check`.
3. Check `drivers/firmware/efi/libstub` has no empty directory leftovers.
4. Run `JOBS=32 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh`.

## Verification result

- Residual scan: clean; no remaining `gop.o`, deleted file path, `efi_setup_graphics`, or `efi_parse_option_graphics` references in source/config scan.
- `git diff --check`: clean.
- Empty directory check: clean for `drivers/firmware/efi/libstub`.
- Fixed storage validation: passed with `JOBS=32 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh`.
- x86_64 log: `.codex-qemu-kernels/logs/qemu/minix-storage-x86_64-20260505-224732-attempt1.log`.
- arm64 log: `.codex-qemu-kernels/logs/qemu/minix-storage-arm64-20260505-224735-attempt1.log`.
- Metrics: `.codex-qemu-kernels/metrics/metrics-20260505-224736.txt`.

## Post-commit review

- Commit: `59c8fc9b8a86 refactor(裁剪firmware): 删除EFI GOP图形helper`.
- `git show --stat --oneline --decorate HEAD`: 6 source files changed, 1 insertion, 585 deletions; deleted `drivers/firmware/efi/libstub/gop.c`.
- `git show --check --oneline HEAD`: clean.
- `git show --name-status --oneline HEAD`: source-only EFI libstub GOP deletion and direct call/build cleanup.
- `git status --short`: clean after source commit.
- Review finding: none.
