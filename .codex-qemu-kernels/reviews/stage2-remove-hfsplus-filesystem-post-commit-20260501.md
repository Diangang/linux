# Post-commit review: stage2-remove-hfsplus-filesystem

Commit: c40939bd99a8 (`fs: remove hfsplus filesystem`)
Patch class: delete_plus_build_wiring

## Scope

Removed the standalone HFSPlus filesystem implementation, private
`include/linux/hfs_common.h` header, documentation, maintainer entry, ioctl table
entry, fs Kconfig/Makefile wiring, statmount known-filesystem token, and stale
CONFIG_HFSPLUS_FS selections from non-scope configs.

## Dependency and contract review

- HFSPlus is a standalone legacy Macintosh filesystem.
- The fixed storage contract keeps Minix plus VFS/core helpers and QEMU NVMe/SCSI
  storage paths for `/dev/nvme0n1` and `/dev/sda`.
- HFS was already removed, so `include/linux/hfs_common.h` no longer had another
  in-tree owner.
- The patch does not change Minix, VFS mount/read/write/sync paths, block core,
  NVMe, SCSI core, SCSI disk, virtio-scsi, PCI, devtmpfs, initramfs, procfs, or
  sysfs runtime logic.
- Other-architecture config edits only remove stale CONFIG_HFSPLUS_FS selections
  for a deleted option and are outside required x86_64/arm64 validation targets.

## Validation

Command:

```sh
JOBS=8 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh
```

Results:

- x86_64 passed generated-initramfs QEMU Minix NVMe/SCSI mount/write/read/sync/umount validation.
- arm64 passed generated-initramfs QEMU Minix NVMe/SCSI mount/write/read/sync/umount validation.
- Logs:
  - `.codex-qemu-kernels/logs/qemu/minix-storage-x86_64-20260501-200337-attempt1.log`
  - `.codex-qemu-kernels/logs/qemu/minix-storage-arm64-20260501-200341-attempt1.log`
- Metrics: `.codex-qemu-kernels/metrics/metrics-20260501-200342.txt`

## Review checks

- `git show --stat --oneline --decorate HEAD`: expected HFSPlus deletion and direct wiring cleanup only.
- `git show --check --oneline HEAD`: clean.
- `git show --name-only --oneline HEAD`: changed files match the selected HFSPlus feature family.
- Targeted `git show -- Documentation/filesystems/index.rst Documentation/userspace-api/ioctl/ioctl-number.rst MAINTAINERS fs/Kconfig fs/Makefile include/linux/stringhash.h tools/testing/selftests/filesystems/statmount/statmount_test.c ...`: only direct HFSPlus references removed.
- `rg -n "\bCONFIG_HFSPLUS_FS\b|\bHFSPLUS_FS\b|fs/hfsplus/|hfsplus|hfs_common" --glob '!.codex-qemu-kernels/**'`: no remaining source references.
- `git status --short --branch`: clean source tree; branch ahead of origin only.

Review status: clean.
