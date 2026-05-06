# stage2-remove-qnx6-filesystem ledger

Date: 2026-05-01
Patch id: stage2-remove-qnx6-filesystem

## Target

Remove the QNX6 filesystem feature family.

## Scope

- Delete the QNX6 implementation under fs/qnx6.
- Delete the QNX6 private on-disk format header include/linux/qnx6_fs.h.
- Delete the QNX6 filesystem documentation and Documentation/filesystems index entry.
- Remove fs/Kconfig and fs/Makefile QNX6 build wiring.
- Remove the QNX6 MAINTAINERS block.
- Drop stale CONFIG_QNX6FS_FS defconfig selections from m68k configs.
- Retain the generic QNX6 magic value in include/uapi/linux/magic.h.

## Rationale

QNX6 is a standalone read-only block filesystem and is outside the fixed validation contract, which keeps Minix plus required VFS/core helpers and QEMU NVMe/SCSI storage paths.

## Mechanical Checks

- Residual scan found no exact CONFIG_QNX6FS_FS, QNX6FS, fs/qnx6, qnx6.rst, qnx6_fs.h, or QNX6 FILESYSTEM references under MAINTAINERS, Documentation, fs, include, or arch.
- MAINTAINERS was checked to preserve the adjacent QORIQ DPAA2 FSL-MC BUS DRIVER block.
- git diff --check passed before validation.
- Patch shape: 25 files changed, 1682 deletions.

## Validation

Pending fixed initramfs Minix storage contract.
