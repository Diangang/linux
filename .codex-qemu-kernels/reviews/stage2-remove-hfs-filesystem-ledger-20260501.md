# Dependency ledger: stage2-remove-hfs-filesystem

Assumptions:

- The fixed storage contract requires Minix, VFS/core helpers, block core, NVMe,
  SCSI core, SCSI disk, virtio-scsi, PCI/interrupt support, devtmpfs, procfs,
  sysfs, and generated initramfs boot paths.
- HFS is a standalone legacy Macintosh filesystem and is not used to format,
  mount, write, read, sync, or unmount `/dev/nvme0n1` or `/dev/sda` in the
  required QEMU validation.
- HFSPlus remains a separate filesystem; `include/linux/hfs_common.h` is kept
  because it is also owned by HFSPlus.

Selected target:

- Remove the complete `fs/hfs/` implementation and direct HFS documentation,
  maintainer, Kconfig/Makefile, statmount, and stale defconfig references.

Patch class:

- `delete_plus_build_wiring`

Expected touched source files:

- `Documentation/filesystems/hfs.rst`
- `Documentation/filesystems/index.rst`
- `MAINTAINERS`
- `fs/Kconfig`
- `fs/Makefile`
- `fs/hfs/*`
- `tools/testing/selftests/filesystems/statmount/statmount_test.c`
- non-scope defconfigs that select `CONFIG_HFS_FS`

Verification before commit:

- Exact reference scan for `CONFIG_HFS_FS`, `HFS_FS`, `fs/hfs/`, and lowercase
  filesystem token `hfs` outside `.codex-qemu-kernels`.
- `git diff --check`.
- `JOBS=8 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh`.

Stop rule:

- If removal requires changing runtime logic, preserving HFS stubs, or altering
  Minix/QEMU storage behavior, set `run_control.stop_condition` to
  `logic_change_required` and stop.
