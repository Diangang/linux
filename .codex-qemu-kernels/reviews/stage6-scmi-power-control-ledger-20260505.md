# Stage 6 SCMI power-control ledger

Target: `drivers/firmware/arm_scmi/`

Selected bucket:

- `drivers/firmware/arm_scmi/scmi_power_control.c`
- Mechanical Kconfig/Makefile/arm64 defconfig references for
  `CONFIG_ARM_SCMI_POWER_CONTROL`

Assumptions:

- The fixed contract is x86_64 plus arm64 QEMU boot with generated initramfs,
  NVMe `/dev/nvme0n1`, virtio-SCSI HDD `/dev/sda`, and Minix
  mount/write/read/sync/unmount.
- arm64 keeps the SCMI core and SMC transport because they are currently built
  and QEMU arm64 boot still reports SCMI protocol bus registration.
- SCMI system power-control handles platform-originated shutdown/reboot
  notifications. It is not part of the required NVMe, SCSI, Minix, initramfs,
  devtmpfs, PCI, or interrupt path.

Evidence:

- `.codex-qemu-kernels/build-arm64/.config` has
  `# CONFIG_ARM_SCMI_POWER_CONTROL is not set`.
- `.codex-qemu-kernels/build-x86_64/.config` does not enable
  `CONFIG_ARM_SCMI_PROTOCOL`.
- Current x86_64 and arm64 build artifacts under
  `.codex-qemu-kernels/build-*/drivers/firmware/arm_scmi/` do not contain
  `scmi_power_control.o`.
- Source references to `CONFIG_ARM_SCMI_POWER_CONTROL` are limited to
  `drivers/firmware/arm_scmi/Kconfig`, `drivers/firmware/arm_scmi/Makefile`,
  and the arm64 defconfig disabled line. `scmi_power_control` references are
  limited to the deleted file and those build/config entries.

Classification:

- `delete_plus_build_wiring`

Deferred:

- `drivers/firmware/arm_scmi/bus.c` still has generic SCMI SystemPower unique
  device handling. That is shared SCMI core logic and would require
  file-internal behavior changes, so it is not touched in this deletion-first
  source patch.

Verification plan:

- `git diff --check`
- `find drivers/firmware/arm_scmi -type d -empty -print`
- `JOBS=32 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh`
- `git show --stat --oneline --decorate HEAD`
- `git show --check --oneline HEAD`

Validation result:

- `JOBS=32 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh`
  passed.
- x86_64 QEMU log:
  `.codex-qemu-kernels/logs/qemu/minix-storage-x86_64-20260505-213523-attempt1.log`
- arm64 QEMU log:
  `.codex-qemu-kernels/logs/qemu/minix-storage-arm64-20260505-213526-attempt1.log`
- Metrics:
  `.codex-qemu-kernels/metrics/metrics-20260505-213527.txt`

Commit review:

- Commit: `35656b41107b`
- `git show --stat --oneline --decorate HEAD` reports 4 source files changed
  with 408 deletions.
- `git show --check --oneline HEAD` reports no whitespace errors.
- The diff deletes the disabled SCMI system power-control source file and its
  Kconfig, Makefile, and arm64 defconfig references only. No storage I/O,
  Minix, NVMe, SCSI, mount/read/write/sync/unmount, or runtime logic was
  changed.

Next:

- Continue `drivers/firmware/` proof pass. SCMI raw debug mode remains disabled
  by `DEBUG_FS=n`, but deleting it requires mechanical removal of dead
  `driver.c`/`bus.c` references, so it needs a separate higher-care ledger.
