# Stage 6 firmware ledger: EFI primary display table

## Target

- Deletion unit: EFI libstub primary-display configuration-table helper
- Source file: `drivers/firmware/efi/libstub/primary_display.c`
- Patch class: `delete_plus_build_wiring`
- Stage: whole-file straggler under `drivers/firmware/efi/libstub/`

## Assumptions

- The fixed validation contract uses serial QEMU boot plus Minix I/O on NVMe `/dev/nvme0n1` and SCSI `/dev/sda`; it does not require EFI framebuffer handoff or graphical console setup.
- Current x86_64 fixed builds do not compile `primary_display.o`; x86 uses `boot_params.screen_info` and `sysfb_primary_display` outside this helper.
- Current arm64 fixed builds compile the helper because `CONFIG_EFI_GENERIC_STUB=y`, but `alloc_primary_display()` returns NULL for arm64 and therefore does not call `__alloc_primary_display()`.
- Removing the standalone helper object and Makefile entry leaves serial console, EFI command-line/initrd loading, random seed, memreserve, secureboot, and storage behavior unchanged.

## Dependency scan

- Build wiring:
  - `drivers/firmware/efi/libstub/Makefile` includes `primary_display.o` in the `CONFIG_EFI_GENERIC_STUB` object list.
- Direct source users:
  - `drivers/firmware/efi/libstub/primary_display.c` defines `__alloc_primary_display()` and a strong `free_primary_display()` implementation for a firmware-installed primary-display table.
  - `drivers/firmware/efi/libstub/efi-stub-entry.c` only calls `__alloc_primary_display()` under `IS_ENABLED(CONFIG_ARM)`, which is false for the fixed arm64 and x86_64 targets.
  - `drivers/firmware/efi/libstub/efi-stub.c` has a weak `free_primary_display()` fallback, so the storage validation paths do not need the strong table-removal helper.
- Runtime evidence:
  - Fixed validation uses serial logs and block-device Minix I/O. It does not inspect framebuffer resources or require EFI GOP/sysfb handoff.
  - Recent arm64 QEMU logs report no UEFI runtime dependency for the fixed boot path, while the arm64 build still verifies EFI-stub link health.

## Planned edit

- Delete `drivers/firmware/efi/libstub/primary_display.c`.
- Remove `primary_display.o` from the EFI generic stub object list.
- Do not remove `sysfb_primary_display`, EFI GOP setup, or primary-display declarations unless the build proves they are direct stale fallout.

## Verification plan

1. Scan for residual `primary_display.o` and deleted file references.
2. Run `git diff --check`.
3. Check `drivers/firmware/efi/libstub` has no empty directory leftovers.
4. Run `JOBS=32 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh`.

## Verification result

- Residual scan: clean; no remaining `primary_display.o` build wiring or deleted file path references in source/config scan.
- `git diff --check`: clean.
- Empty directory check: clean for `drivers/firmware/efi/libstub`.
- Fixed storage validation: passed with `JOBS=32 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh`.
- x86_64 log: `.codex-qemu-kernels/logs/qemu/minix-storage-x86_64-20260505-224259-attempt1.log`.
- arm64 log: `.codex-qemu-kernels/logs/qemu/minix-storage-arm64-20260505-224302-attempt1.log`.
- Metrics: `.codex-qemu-kernels/metrics/metrics-20260505-224303.txt`.

## Post-commit review

- Commit: `15a05bf1f797 refactor(裁剪firmware): 删除EFI primary display helper`.
- `git show --stat --oneline --decorate HEAD`: 2 source files changed, 1 insertion, 56 deletions; deleted `drivers/firmware/efi/libstub/primary_display.c`.
- `git show --check --oneline HEAD`: clean.
- `git show --name-status --oneline HEAD`: source-only EFI libstub helper deletion and Makefile cleanup.
- `git status --short`: clean after source commit.
- Review finding: none.
