# Stage 6 firmware ledger: DMI ID sysfs exporter

## Target

- Deletion unit: DMI identification sysfs exporter
- Source file: `drivers/firmware/dmi-id.c`
- Patch class: `delete_plus_build_wiring`
- Stage: whole-file straggler under `drivers/firmware/`

## Assumptions

- The fixed validation contract uses generated initramfs boot and Minix I/O on QEMU NVMe plus SCSI for x86_64 and arm64.
- The contract does not require `/sys/class/dmi/id`, DMI modalias uevents, or userspace SMBIOS identification export.
- DMI scanning itself remains needed for x86/ACPI/EFI runtime and platform quirks, so `drivers/firmware/dmi_scan.c` and `CONFIG_DMI` are kept.
- Removing `dmi-id.c` does not alter NVMe, SCSI, block, VFS, Minix, device names, serial console, DMI table scanning, or EFI runtime behavior.

## Dependency scan

- Build wiring:
  - `drivers/firmware/Makefile` builds `dmi-id.o` under `CONFIG_DMIID`.
  - `drivers/firmware/Kconfig` defines `CONFIG_DMIID`.
  - `arch/x86/configs/x86_64_defconfig` and `arch/arm64/configs/defconfig` set `CONFIG_DMIID=y`.
- Direct source users:
  - Search found no non-local callers of `dmi_id_init`, `sys_dmi_*`, `dmi_class`, or `dmi_dev`.
  - `dmi-id.c` only registers a DMI class device and sysfs attributes from `arch_initcall`.
- Kept dependencies:
  - `drivers/firmware/dmi_scan.c` owns `dmi_available`, DMI identification storage, `dmi_get_system_info()`, DMI table scan, and `dmi_kobj`.
  - Existing DMI quirk users in ACPI/x86 and EFI runtime comments remain served by `dmi_scan.c`.

## Planned edit

- Delete `drivers/firmware/dmi-id.c`.
- Remove `CONFIG_DMIID` and its `dmi-id.o` Makefile entry.
- Remove stale fixed defconfig selections for x86_64 and arm64.
- Leave `CONFIG_DMI`, `dmi_scan.c`, and all DMI scan APIs intact.

## Verification plan

1. Scan for residual `CONFIG_DMIID`, `dmi-id.o`, deleted file path, and local DMI ID sysfs symbols.
2. Run `git diff --check`.
3. Check `drivers/firmware` has no empty directory leftovers.
4. Run `JOBS=32 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh`.

## Verification result

- Residual scan after validation: clean for `CONFIG_DMIID`, `dmi-id.o`, deleted file path, `dmi_id_init`, local DMI ID sysfs symbols, and `/sys/class/dmi/id` references.
- `git diff --check`: clean.
- Empty directory check: `drivers/firmware` has no empty directory leftovers.
- Fixed storage contract: passed with `JOBS=32 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh`.
  - x86_64 log: `.codex-qemu-kernels/logs/qemu/minix-storage-x86_64-20260505-225607-attempt1.log`
  - arm64 log: `.codex-qemu-kernels/logs/qemu/minix-storage-arm64-20260505-225610-attempt1.log`
  - metrics: `.codex-qemu-kernels/metrics/metrics-20260505-225611.txt`

## Post-commit review

- Commit: `ec2913c3c795 refactor(裁剪firmware): 删除DMI ID sysfs导出`
- `git show --stat --oneline --decorate HEAD`: expected six-file patch, deleting `drivers/firmware/dmi-id.c` and only direct Kconfig/Makefile/defconfig/comment fallout.
- `git show --check --oneline HEAD`: clean.
- `git show --name-status --oneline HEAD`: one deleted source file and five direct wiring/comment edits.
- `git status --short`: clean after committing kernel source changes only.
- Review result: clean.
