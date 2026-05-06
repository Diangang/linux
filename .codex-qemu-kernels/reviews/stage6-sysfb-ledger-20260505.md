# Stage 6 SYSFB ledger

Target: generic system framebuffer platform-device support

Selected bucket:

- `drivers/firmware/sysfb.c`
- `drivers/firmware/efi/sysfb_efi.c`
- Direct SYSFB Kconfig, Makefile, EFI stub condition, and header wiring

Assumptions:

- The fixed contract is x86_64 plus arm64 QEMU boot with generated initramfs,
  NVMe `/dev/nvme0n1`, virtio-SCSI HDD `/dev/sda`, and Minix
  mount/write/read/sync/unmount.
- SYSFB registers firmware framebuffer platform devices and EFI framebuffer
  quirks; it is not part of serial-console boot logging or block storage I/O.
- `sysfb_display_info` and `sysfb_primary_display` remain because EFI earlycon
  and EFI stub code still use the primary display handoff.

Evidence:

- Neither fixed config enables `CONFIG_SYSFB`.
- The current x86_64 and arm64 build object inventories contain no `sysfb.o`
  or `sysfb_efi.o`.
- `drivers/firmware/sysfb.c` registers framebuffer platform devices at
  device-init time.
- `drivers/firmware/efi/sysfb_efi.c` implements EFI framebuffer DMI/PCI
  quirks for the SYSFB path.
- The fixed QEMU validation uses serial consoles (`ttyS0` and `ttyAMA0`) and
  has no framebuffer requirement.

Classification:

- `delete_plus_build_wiring`

Deferred:

- `struct sysfb_display_info`, `sysfb_primary_display`, and EFI earlycon
  primary-display handling remain because they are still used by EFI earlycon
  and EFI stub code.
- Generic `screen_info` support remains because x86 boot, VGA console, kexec,
  and EFI stub paths still reference it.

Verification plan:

- `rg -n "drivers/firmware/sysfb.c|sysfb_efi.c|CONFIG_SYSFB|obj-\\$\\(CONFIG_SYSFB\\)|efifb_setup_from_dmi|sysfb_apply_efi_quirks|sysfb_set_efifb_fwnode|sysfb_handles_screen_info" arch drivers include`
- `git diff --check`
- `find drivers/firmware drivers/firmware/efi -type d -empty -print`
- `JOBS=32 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh`
- `git show --stat --oneline --decorate HEAD`
- `git show --check --oneline HEAD`

Validation:

- Residual removed-symbol scan:
  `rg -n "drivers/firmware/sysfb.c|sysfb_efi.c|CONFIG_SYSFB|obj-\\$\\(CONFIG_SYSFB\\)|efifb_setup_from_dmi|sysfb_apply_efi_quirks|sysfb_set_efifb_fwnode|sysfb_handles_screen_info|efifb_dmi_info|efifb_dmi_list" arch drivers include`
  produced no matches.
- `git diff --check` produced no output.
- `find drivers/firmware drivers/firmware/efi -type d -empty -print`
  produced no output.
- Fixed storage contract passed:
  `JOBS=32 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh`
- x86_64 QEMU log:
  `.codex-qemu-kernels/logs/qemu/minix-storage-x86_64-20260505-221558-attempt1.log`
- arm64 QEMU log:
  `.codex-qemu-kernels/logs/qemu/minix-storage-arm64-20260505-221601-attempt1.log`
- Metrics:
  `.codex-qemu-kernels/metrics/metrics-20260505-221602.txt`

Review before commit:

- Source diff deletes the SYSFB source files and direct config/build/API
  wiring while preserving `sysfb_display_info` and `sysfb_primary_display` for
  EFI earlycon and EFI stub handoff.
- Patch classification remains `delete_plus_build_wiring`.

Commit:

- `4ee8cb2de8b9 refactor(裁剪firmware): 删除SYSFB平台帧缓冲支持`

Post-commit review:

- `git show --stat --oneline --decorate HEAD` reports only the selected SYSFB
  source files and direct config/build/API wiring: 10 files changed, 3
  insertions, 773 deletions.
- The retained insertions are the mechanically shortened EFI earlycon/stub
  conditions after removing `CONFIG_SYSFB`.
- `git show --check --oneline HEAD` produced no whitespace or patch-format
  findings.
- `git status --short` was clean for tracked source files.

Review result:

- Clean. Patch remains `delete_plus_build_wiring`; no runtime storage, Minix,
  NVMe, SCSI, mount, read/write, sync, or device-name behavior was changed.
