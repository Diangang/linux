# Stage 1 validation automation ledger

Target: automate the fixed QEMU Minix storage validation before source
trimming.

Patch class: control_workspace_only

Source changes: none.

Dependency ledger:

- x86_64 build uses `x86_64_defconfig` plus the contract-required NVMe, Minix,
  virtio block, virtio PCI, virtio SCSI, devtmpfs, and devtmpfs mount options.
- arm64 build uses `defconfig` plus the same contract-required storage and
  device-node options, with `aarch64-linux-gnu-` cross tools.
- QEMU storage test keeps the fixed devices from `QEMU_KERNEL_MINIX_TEST.md`:
  `/dev/nvme0n1` for the NVMe disk and `/dev/sda` for the SCSI HDD-like disk.
- Guest commands preserve the required Minix operations: `mkfs.minix -3`,
  mount, write, read, sync, and unmount on both devices.
- Generated overlays, disks, logs, guest command files, build trees, and
  metrics are all under `.codex-qemu-kernels/`.

Deletion-policy review:

- No kernel source files are modified.
- No runtime behavior or kernel logic is changed.
- This enables later deletion-first rounds by making the hard storage contract
  repeatable.
