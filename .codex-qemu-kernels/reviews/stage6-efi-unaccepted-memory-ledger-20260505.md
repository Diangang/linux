# Stage 6 firmware ledger: EFI unaccepted memory support

## Target

- Deletion unit: EFI unaccepted memory support for confidential guests
- Source files:
  - `drivers/firmware/efi/unaccepted_memory.c`
  - `drivers/firmware/efi/libstub/unaccepted_memory.c`
  - `drivers/firmware/efi/libstub/bitmap.c`
  - `drivers/firmware/efi/libstub/find.c`
- Patch class: `delete_plus_build_wiring`
- Stage: whole-file straggler under `drivers/firmware/efi/`

## Assumptions

- The fixed validation contract uses x86_64 and arm64 QEMU TCG storage boot, generated initramfs, and Minix I/O on NVMe plus SCSI.
- The fixed x86_64 config has `CONFIG_INTEL_TDX_GUEST` and `CONFIG_AMD_MEM_ENCRYPT` disabled; arm64 does not select `CONFIG_UNACCEPTED_MEMORY`.
- EFI unaccepted memory handling is for confidential guest platforms, not for QEMU TCG NVMe/SCSI Minix validation.
- Existing disabled-config stubs in generic MM headers cover callers when unaccepted memory is not configured.

## Dependency scan

- Build wiring:
  - `drivers/firmware/efi/Makefile` builds `unaccepted_memory.o` under `CONFIG_UNACCEPTED_MEMORY`.
  - `drivers/firmware/efi/libstub/Makefile` builds `unaccepted_memory.o bitmap.o find.o` under `CONFIG_UNACCEPTED_MEMORY`.
  - `drivers/firmware/efi/Kconfig` defines `CONFIG_UNACCEPTED_MEMORY`.
  - `arch/x86/Kconfig` selects `UNACCEPTED_MEMORY` from disabled TDX/AMD encrypted guest options in the fixed config.
  - `arch/x86/boot/compressed/Makefile` builds compressed `mem.o` under `CONFIG_UNACCEPTED_MEMORY`.
- Direct source users:
  - EFI core and x86 EFI stub have guarded unaccepted-memory paths.
  - Generic MM/page allocator callers are guarded by `CONFIG_UNACCEPTED_MEMORY` and use existing no-op behavior when it is absent.
- Kept dependencies:
  - EFI memory map parsing, EFI soft reserve, DMI, PSCI, SMCCC, NVMe, SCSI, block, VFS, and Minix paths remain intact.

## Planned edit

- Delete the four unaccepted-memory/libstub helper files.
- Remove direct Kconfig and Makefile build/select entries for `CONFIG_UNACCEPTED_MEMORY`.
- Remove direct EFI stub/core declarations and guarded branches that would otherwise reference deleted helpers.
- Leave unrelated EFI memory-map, soft-reserve, secureboot, random seed, command-line/initrd, and storage paths unchanged.

## Verification plan

1. Scan for residual deleted file paths, `unaccepted_memory.o`, `bitmap.o find.o` unaccepted libstub wiring, and deleted helper definitions.
2. Run `git diff --check`.
3. Check `drivers/firmware/efi` and `drivers/firmware/efi/libstub` have no empty directory leftovers.
4. Run `JOBS=32 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh`.

## Verification result

- Residual scan after validation: clean for deleted unaccepted-memory file paths, deleted helper definitions, deleted Kconfig/select/build wiring, and EFI `efi.unaccepted` references. The only remaining scan hits were unrelated ACPI `tbfind.o` names.
- `git diff --check`: clean.
- Empty directory check: affected EFI/libstub and x86 directories have no empty directory leftovers.
- Fixed storage contract: passed with `JOBS=32 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh`.
  - x86_64 log: `.codex-qemu-kernels/logs/qemu/minix-storage-x86_64-20260505-230346-attempt1.log`
  - arm64 log: `.codex-qemu-kernels/logs/qemu/minix-storage-arm64-20260505-230350-attempt1.log`
  - metrics: `.codex-qemu-kernels/metrics/metrics-20260505-230351.txt`

## Commit and stop analysis

- Source commit: `e8492b4d13ff` (`refactor(裁剪firmware): 删除EFI unaccepted memory支持`).
- Post-validation supervisor stop cause: external Codex usage-limit / remote-compaction failure, not a kernel validation failure.
- Bugfix rounds did not run kernel analysis or produce a source finding; they exited immediately with the same usage-limit error.
- No evidence of `logic_change_required`: the accepted source change is deletion plus stale Kconfig/Makefile/declaration/callsite cleanup only.
