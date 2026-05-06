# Stage 6 firmware ledger: firmware memmap sysfs exporter

## Target

- Deletion unit: firmware-provided memory map sysfs exporter
- Source file: `drivers/firmware/memmap.c`
- Patch class: `delete_plus_build_wiring`
- Stage: whole-file straggler under `drivers/firmware/`

## Assumptions

- The fixed validation contract uses x86_64 and arm64 QEMU storage boot, generated initramfs, and Minix I/O on NVMe plus SCSI.
- The contract does not require `/sys/firmware/memmap`, kexec-tools consumption of raw firmware maps, or memory-hotplug sysfs map maintenance.
- Core e820, memblock, hotplug, and boot memory behavior must remain unchanged for the fixed boot path.
- `include/linux/firmware-map.h` already provides no-op stubs when `CONFIG_FIRMWARE_MEMMAP` is disabled, so callers can remain intact without runtime logic edits.

## Dependency scan

- Build wiring:
  - `drivers/firmware/Makefile` builds `memmap.o` under `CONFIG_FIRMWARE_MEMMAP`.
  - `drivers/firmware/Kconfig` defines `CONFIG_FIRMWARE_MEMMAP`, defaulting to x86.
  - `arch/x86/configs/x86_64_defconfig` sets `CONFIG_FIRMWARE_MEMMAP=y`.
- Direct source users:
  - `arch/x86/kernel/e820.c` calls `firmware_map_add_early()` to export e820 entries.
  - `mm/memory_hotplug.c` calls `firmware_map_add_hotplug()` and `firmware_map_remove()`.
  - These calls are covered by existing no-op stubs when the config is absent.
- Kept dependencies:
  - `include/linux/firmware-map.h` remains because it is the existing compile-time stub boundary.
  - e820 memory discovery, ACPI/EFI memory maps, and storage paths remain intact.

## Planned edit

- Delete `drivers/firmware/memmap.c`.
- Remove `CONFIG_FIRMWARE_MEMMAP`, its Makefile object, and stale x86_64 defconfig selection.
- Leave caller code and `include/linux/firmware-map.h` stubs intact.

## Verification plan

1. Scan for residual `CONFIG_FIRMWARE_MEMMAP`, `memmap.o` firmware build wiring, deleted file path, and non-stub firmware map definitions.
2. Run `git diff --check`.
3. Check `drivers/firmware` has no empty directory leftovers.
4. Run `JOBS=32 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh`.

## Verification result

- Residual scan after validation: clean for deleted file path, `memmap.o` firmware build wiring, and non-stub firmware map definitions; only the intentional disabled-stub boundary in `include/linux/firmware-map.h` remains.
- `git diff --check`: clean.
- Empty directory check: `drivers/firmware` has no empty directory leftovers.
- Fixed storage contract: passed with `JOBS=32 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh`.
  - x86_64 log: `.codex-qemu-kernels/logs/qemu/minix-storage-x86_64-20260505-225839-attempt1.log`
  - arm64 log: `.codex-qemu-kernels/logs/qemu/minix-storage-arm64-20260505-225842-attempt1.log`
  - metrics: `.codex-qemu-kernels/metrics/metrics-20260505-225843.txt`

## Post-commit review

- Commit: `e3eba7d12ad8 refactor(裁剪firmware): 删除firmware memmap导出`
- `git show --stat --oneline --decorate HEAD`: expected four-file patch, deleting `drivers/firmware/memmap.c` and only direct Kconfig/Makefile/x86 defconfig wiring.
- `git show --check --oneline HEAD`: clean.
- `git show --name-status --oneline HEAD`: one deleted source file and three direct wiring edits.
- `git status --short`: clean after committing kernel source changes only.
- Review result: clean.
