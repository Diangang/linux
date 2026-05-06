# Post-commit review: stage2-remove-hfs-filesystem

Commit: 3ee40da4df38 (`fs: remove hfs filesystem`)
Patch class: delete_plus_build_wiring

## Scope

Removed the standalone HFS filesystem implementation, documentation, maintainer
entry, fs Kconfig/Makefile wiring, statmount known-filesystem token, CREDITS
line, and stale CONFIG_HFS_FS selections from non-scope configs.

## Dependency and contract review

- HFS is a standalone legacy Macintosh filesystem.
- The fixed storage contract keeps Minix plus VFS/core helpers and QEMU NVMe/SCSI
  storage paths for `/dev/nvme0n1` and `/dev/sda`.
- HFSPlus remains present; `include/linux/hfs_common.h` was retained because it
  is also owned by HFSPlus.
- The patch does not change Minix, VFS mount/read/write/sync paths, block core,
  NVMe, SCSI core, SCSI disk, virtio-scsi, PCI, devtmpfs, initramfs, procfs, or
  sysfs runtime logic.
- Other-architecture config edits only remove stale CONFIG_HFS_FS selections for
  a deleted option and are outside required x86_64/arm64 validation targets.

## Validation

Command:

```sh
JOBS=8 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh
```

Results:

- x86_64 passed generated-initramfs QEMU Minix NVMe/SCSI mount/write/read/sync/umount validation.
- arm64 passed generated-initramfs QEMU Minix NVMe/SCSI mount/write/read/sync/umount validation.
- Logs:
  - `.codex-qemu-kernels/logs/qemu/minix-storage-x86_64-20260501-194619-attempt1.log`
  - `.codex-qemu-kernels/logs/qemu/minix-storage-arm64-20260501-194623-attempt1.log`
- Metrics: `.codex-qemu-kernels/metrics/metrics-20260501-194625.txt`

## Review checks

- `git show --stat --oneline --decorate HEAD`: expected HFS deletion and direct wiring cleanup only.
- `git show --check --oneline HEAD`: clean.
- `git show --name-only --oneline HEAD`: changed files match the selected HFS feature family.
- Targeted `git show -- CREDITS Documentation/filesystems/index.rst MAINTAINERS fs/Kconfig fs/Makefile tools/testing/selftests/filesystems/statmount/statmount_test.c ...`: only direct HFS references removed.
- `rg -n "\bCONFIG_HFS_FS\b|\bHFS_FS\b|fs/hfs/" --glob '!.codex-qemu-kernels/**'`: no remaining source references.
- `rg -n "\bhfs\b" --glob '!.codex-qemu-kernels/**'`: only shared HFSPlus/common text and historical HP-UX comments remain.
- `git status --short --branch`: clean source tree; branch ahead of origin only.

Review status: clean.
