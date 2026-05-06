# Stage 6 EFI/ACPI BGRT ledger

Target: EFI/ACPI Boot Graphics Resource Table support

Selected bucket:

- `drivers/firmware/efi/efi-bgrt.c`
- `drivers/acpi/bgrt.c`
- `include/linux/efi-bgrt.h`
- Direct ACPI/EFI Makefile, Kconfig, defconfig, and BGRT parse wiring

Assumptions:

- The fixed contract is x86_64 plus arm64 QEMU boot with generated initramfs,
  NVMe `/dev/nvme0n1`, virtio-SCSI HDD `/dev/sda`, and Minix
  mount/write/read/sync/unmount.
- BGRT exposes firmware boot-splash image metadata under
  `/sys/firmware/acpi/bgrt/`; it is not part of block discovery, NVMe, SCSI,
  Minix, mount, read/write, sync, or device-name behavior.
- Removing the `bgrt_disable` command-line hook is direct feature deletion
  fallout because that parameter only controls BGRT parsing.

Evidence:

- `.codex-qemu-kernels/build-x86_64/.config` has `CONFIG_ACPI_BGRT=y`, and
  x86_64 currently builds `drivers/acpi/bgrt.o` and
  `drivers/firmware/efi/efi-bgrt.o`.
- `.codex-qemu-kernels/build-arm64/.config` has
  `# CONFIG_ACPI_BGRT is not set`, so arm64 does not build BGRT.
- The latest fixed QEMU logs contain no `BGRT`/`bgrt` matches.
- `drivers/acpi/Kconfig` describes BGRT as boot graphic sysfs exposure.
- `drivers/acpi/bgrt.c` creates the `/sys/firmware/acpi/bgrt/` attribute
  group and calls `efi_bgrt_init()`.
- `drivers/firmware/efi/efi-bgrt.c` validates/reserves the firmware boot
  bitmap referenced by the ACPI BGRT table.

Classification:

- `delete_plus_build_wiring`

Deferred:

- Generic ACPI table signature definitions remain in `include/acpi/actbl1.h`
  and ACPI table listing code because they are broad ACPI table metadata, not
  BGRT runtime support.

Verification plan:

- `rg -n "efi-bgrt.h|acpi_parse_bgrt|efi_bgrt_init|bgrt_disable|acpi_nobgrt|CONFIG_ACPI_BGRT|ACPI_BGRT|efi-bgrt|bgrt.o" arch drivers include`
- `git diff --check`
- `find drivers/firmware/efi drivers/acpi -type d -empty -print`
- `JOBS=32 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh`
- `git show --stat --oneline --decorate HEAD`
- `git show --check --oneline HEAD`

Validation:

- Residual removed-symbol scan:
  `rg -n "efi-bgrt.h|acpi_parse_bgrt|efi_bgrt_init|bgrt_disable|acpi_nobgrt|CONFIG_ACPI_BGRT|efi-bgrt|bgrt.o" arch drivers include`
  produced no matches.
- `git diff --check` produced no output.
- `find drivers/firmware/efi drivers/acpi -type d -empty -print` produced no
  output.
- Fixed storage contract passed:
  `JOBS=32 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh`
- x86_64 QEMU log:
  `.codex-qemu-kernels/logs/qemu/minix-storage-x86_64-20260505-221138-attempt1.log`
- arm64 QEMU log:
  `.codex-qemu-kernels/logs/qemu/minix-storage-arm64-20260505-221141-attempt1.log`
- Metrics:
  `.codex-qemu-kernels/metrics/metrics-20260505-221142.txt`

Review before commit:

- Source diff is limited to deleting BGRT source/header files and their direct
  ACPI/EFI Kconfig, Makefile, defconfig, command-line, and parse-call wiring.
- Patch classification remains `delete_plus_build_wiring`.

Commit:

- `161535340c85 refactor(裁剪firmware): 删除EFI ACPI BGRT支持`

Post-commit review:

- `git show --stat --oneline --decorate HEAD` reports only the selected BGRT
  source/header files and direct ACPI/EFI/arch wiring: 11 files changed, 239
  deletions.
- `git show --check --oneline HEAD` produced no whitespace or patch-format
  findings.
- `git status --short` was clean for tracked source files.

Review result:

- Clean. Patch remains `delete_plus_build_wiring`; no runtime storage, Minix,
  NVMe, SCSI, mount, read/write, sync, or device-name behavior was changed.
