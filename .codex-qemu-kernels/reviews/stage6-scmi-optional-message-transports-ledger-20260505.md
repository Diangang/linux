# Stage 6 SCMI optional message transports ledger

Target: `drivers/firmware/arm_scmi/`

Selected bucket:

- `drivers/firmware/arm_scmi/transports/mailbox.c`
- `drivers/firmware/arm_scmi/transports/optee.c`
- `drivers/firmware/arm_scmi/transports/virtio.c`
- `drivers/firmware/arm_scmi/msg.c`
- Mechanical Kconfig/Makefile/arm64 defconfig references for the removed symbols

Assumptions:

- The fixed contract is x86_64 plus arm64 QEMU boot with generated initramfs,
  NVMe `/dev/nvme0n1`, virtio-SCSI HDD `/dev/sda`, Minix mount/write/read/sync/
  unmount.
- arm64 keeps `CONFIG_ARM_SCMI_PROTOCOL=y` and
  `CONFIG_ARM_SCMI_TRANSPORT_SMC=y`; the SMC transport remains in
  `drivers/firmware/arm_scmi/transports/`.
- The mailbox, OP-TEE, and SCMI virtio transports are not required for the fixed
  QEMU storage contract. The validation virtio dependency is virtio-SCSI and
  virtio-PCI, not SCMI-over-virtio.

Evidence:

- `.codex-qemu-kernels/build-arm64/.config` has
  `CONFIG_ARM_SCMI_PROTOCOL=y`, `CONFIG_ARM_SCMI_TRANSPORT_SMC=y`,
  `# CONFIG_ARM_SCMI_TRANSPORT_VIRTIO is not set`, and
  `# CONFIG_ARM_SCMI_POWER_CONTROL is not set`.
- `.codex-qemu-kernels/build-x86_64/.config` does not enable
  `CONFIG_ARM_SCMI_PROTOCOL`.
- Current build artifacts show arm64 builds `transports/smc.o`; no x86_64 or
  arm64 artifacts build `mailbox.o`, `optee.o`, `virtio.o`, or `msg.o`.
- `drivers/firmware/arm_scmi/transports/Kconfig` selects
  `ARM_SCMI_HAVE_MSG` only from the OP-TEE and virtio transports. Removing those
  leaves `drivers/firmware/arm_scmi/msg.c` without a build selector.

Classification:

- `delete_plus_build_wiring`

Deferred:

- `raw_mode.c` is disabled with `DEBUG_FS=n`, but its header and calls are still
  compiled through `driver.c` dead branches. Deleting it would require driver
  internal cleanup, so it is deferred to a separate proof.
- `scmi_power_control.c` is disabled and remains a separate optional SCMI
  feature proof.

Verification plan:

- `.codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh`
- `git show --stat --oneline --decorate HEAD`
- `git show --check --oneline HEAD`
- Review the committed diff for source-only changes and no runtime logic edits.

Validation result:

- `JOBS=32 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh`
  passed.
- x86_64 QEMU log:
  `.codex-qemu-kernels/logs/qemu/minix-storage-x86_64-20260505-213219-attempt1.log`
- arm64 QEMU log:
  `.codex-qemu-kernels/logs/qemu/minix-storage-arm64-20260505-213222-attempt1.log`
- Metrics:
  `.codex-qemu-kernels/metrics/metrics-20260505-213223.txt`

Commit review:

- Commit: `8b9960aa5f2a`
- `git show --stat --oneline --decorate HEAD` reports 8 source files changed
  with 2169 deletions.
- `git show --check --oneline HEAD` reports no whitespace errors.
- The diff deletes SCMI mailbox, OP-TEE, virtio message transports,
  `msg.c`, and their build/config wiring only. No storage I/O, mount, Minix,
  NVMe, SCSI, or runtime control-flow logic was changed.

Next:

- Continue `drivers/firmware/` proof pass with a separate ledger for another
  coherent optional SCMI/firmware bucket, such as disabled SCMI power-control,
  before considering any file-internal cleanup.
