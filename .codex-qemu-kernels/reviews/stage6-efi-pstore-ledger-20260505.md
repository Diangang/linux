# Stage 6 firmware ledger: EFI pstore backend

## Target

- Deletion unit: `drivers/firmware/efi/efi-pstore.c`
- Patch class: `delete_plus_build_wiring`
- Stage: whole-file straggler under `drivers/firmware/efi/`

## Assumptions

- The fixed storage contract only requires x86_64 and arm64 QEMU to boot a generated initramfs and exercise NVMe plus SCSI Minix disks.
- The user-retained pstore support is `fs/pstore/` plus `CONFIG_PSTORE_RAM`; this patch does not remove pstore core or ramoops.
- EFI-variable pstore stores logs in EFI variables and is not part of Minix mount/write/read/sync/unmount on `/dev/nvme0n1` or `/dev/sda`.
- The fixed arm64 QEMU log reports `efi: UEFI not found.`, so this backend is unavailable in the validation boot path.

## Dependency scan

- Build wiring:
  - `drivers/firmware/efi/Makefile` builds `efi-pstore.o` from `CONFIG_EFI_VARS_PSTORE`.
  - `drivers/firmware/efi/Kconfig` defines `EFI_VARS_PSTORE` and `EFI_VARS_PSTORE_DEFAULT_DISABLE`.
  - `arch/arm64/configs/defconfig` requests `CONFIG_EFI_VARS_PSTORE=y`; x86_64 has `CONFIG_PSTORE` disabled.
- Direct source users:
  - `rg` finds `efi_pstore` implementation symbols only inside `drivers/firmware/efi/efi-pstore.c`.
  - External pstore core references remain generic and are not removed.
- Runtime evidence:
  - Recent x86_64 validation has `CONFIG_PSTORE` disabled.
  - Recent arm64 validation booted without UEFI services and still passed the fixed NVMe/SCSI Minix test.

## Planned edit

- Delete `drivers/firmware/efi/efi-pstore.c`.
- Remove `EFI_VARS_PSTORE` and `EFI_VARS_PSTORE_DEFAULT_DISABLE` Kconfig entries.
- Remove `efi-pstore.o` Makefile wiring.
- Remove `CONFIG_EFI_VARS_PSTORE=y` and its default-disable line from arm64 defconfig.

## Verification plan

1. Scan for residual `CONFIG_EFI_VARS_PSTORE`, `EFI_VARS_PSTORE`, `efi_pstore`, and `efi-pstore.o` source references.
2. Run `git diff --check`.
3. Check `drivers/firmware/efi` has no empty directory leftovers.
4. Run `JOBS=32 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh`.

## Verification result

- Residual scan after validation found no `CONFIG_EFI_VARS_PSTORE`, `EFI_VARS_PSTORE`, `efi_pstore`, `efi-pstore.o`, or `efi-pstore` references in source or regenerated fixed configs.
- `git diff --check` passed.
- `find drivers/firmware/efi -type d -empty -print` produced no empty directories.
- Fixed storage validation passed with generated initramfs and `rdinit=/init`:
  - x86_64 QEMU log: `.codex-qemu-kernels/logs/qemu/minix-storage-x86_64-20260505-223237-attempt1.log`
  - arm64 QEMU log: `.codex-qemu-kernels/logs/qemu/minix-storage-arm64-20260505-223240-attempt1.log`
  - metrics: `.codex-qemu-kernels/metrics/metrics-20260505-223241.txt`

## Post-commit review

- Commit: `a2a213885ad2 refactor(裁剪firmware): 删除EFI pstore后端`
- `git show --stat --oneline --decorate HEAD` matched the intended 4-file source patch: one deleted EFI pstore source file plus direct Kconfig, Makefile, and arm64 defconfig cleanup.
- `git show --check --oneline HEAD` reported no whitespace errors.
- `git show --name-status --oneline HEAD` showed no unrelated subsystem changes.
- `git status --short` was clean for kernel source after the commit.
