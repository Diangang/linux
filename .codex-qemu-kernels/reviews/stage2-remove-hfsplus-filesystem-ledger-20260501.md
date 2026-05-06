# Dependency ledger: stage2-remove-hfsplus-filesystem

Assumptions:

- The fixed storage contract requires Minix, VFS/core helpers, block core, NVMe,
  SCSI core, SCSI disk, virtio-scsi, PCI/interrupt support, devtmpfs, procfs,
  sysfs, and generated initramfs boot paths.
- HFSPlus is a standalone legacy Macintosh filesystem and is not used to format,
  mount, write, read, sync, or unmount `/dev/nvme0n1` or `/dev/sda` in the
  required QEMU validation.
- HFS was removed in the previous commit, so `include/linux/hfs_common.h` is now
  HFSPlus-only and can be deleted with the HFSPlus feature family.

Selected target:

- Remove the complete `fs/hfsplus/` implementation and direct HFSPlus
  documentation, maintainer, Kconfig/Makefile, ioctl documentation, statmount,
  shared private header, and stale config references.

Patch class:

- `delete_plus_build_wiring`

Verification before commit:

- Exact reference scan for `CONFIG_HFSPLUS_FS`, `HFSPLUS_FS`, `fs/hfsplus/`,
  `hfsplus`, and `hfs_common` outside `.codex-qemu-kernels`.
- `git diff --check`.
- `JOBS=8 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh`.

Stop rule:

- If removal requires changing runtime logic, preserving HFSPlus stubs, or
  altering Minix/QEMU storage behavior, set `run_control.stop_condition` to
  `logic_change_required` and stop.
