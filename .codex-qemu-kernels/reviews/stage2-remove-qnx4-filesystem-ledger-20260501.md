# Dependency ledger: stage2-remove-qnx4-filesystem

Date: 2026-05-01
Patch class: delete_plus_build_wiring

## Target

Remove the QNX4 filesystem implementation as one complete Stage 2 filesystem family.

## Dependency check

Keep list comparison:

- Fixed validation filesystem is Minix only.
- Required block devices are QEMU NVMe `/dev/nvme0n1` and SCSI HDD `/dev/sda`.
- QNX4 is a separate miscellaneous filesystem and is not used by the initramfs validation flow.

Reference scan:

```sh
rg -n "CONFIG_QNX4FS_FS|QNX4FS|fs/qnx4|source \"fs/qnx4/Kconfig\"" MAINTAINERS fs arch -S
```

After the patch, the scan produced no matches.

UAPI note:

- `include/uapi/linux/qnx4_fs.h`, `include/uapi/linux/qnxtypes.h`, and the `QNX4_SUPER_MAGIC` definition are retained, matching the prior BFS handling where UAPI definitions remain while the filesystem implementation is removed.

## Source changes

- Delete `fs/qnx4/`.
- Remove `fs/Kconfig` and `fs/Makefile` QNX4 wiring.
- Remove the MAINTAINERS `fs/qnx4/` pattern while retaining QNX4 UAPI patterns.
- Remove stale `CONFIG_QNX4FS_FS=m` selections from m68k, MIPS, and PowerPC defconfigs.

## Pre-validation review

- `git diff --stat`: 25 files changed, 824 deletions.
- `git diff --check`: passed.

Finding: ready for required x86_64 and arm64 fixed initramfs Minix storage validation.
