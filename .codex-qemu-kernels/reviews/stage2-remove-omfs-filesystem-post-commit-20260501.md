# Post-commit review: stage2-remove-omfs-filesystem

Commit: a4416883fb99 (`fs: remove omfs filesystem`)
Patch class: delete_plus_build_wiring

## Scope

Removed the standalone OMFS filesystem implementation, documentation, maintainer
entry, fs Kconfig/Makefile wiring, statmount known-filesystem token, and stale
CONFIG_OMFS_FS selections from non-scope defconfigs.

## Dependency and contract review

- OMFS is a standalone filesystem for legacy SonicBlue/Rio Karma/ReplayTV media.
- The fixed storage contract keeps Minix plus VFS/core helpers and QEMU NVMe/SCSI
  storage paths for `/dev/nvme0n1` and `/dev/sda`.
- The patch does not change Minix, VFS mount/read/write/sync paths, block core,
  NVMe, SCSI core, SCSI disk, virtio-scsi, PCI, devtmpfs, initramfs, procfs, or
  sysfs runtime logic.
- Other-architecture defconfig edits only remove stale CONFIG_OMFS_FS selections
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
  - `.codex-qemu-kernels/logs/qemu/minix-storage-x86_64-20260501-192823-attempt1.log`
  - `.codex-qemu-kernels/logs/qemu/minix-storage-arm64-20260501-192828-attempt1.log`
- Metrics: `.codex-qemu-kernels/metrics/metrics-20260501-192829.txt`

## Review checks

- `git show --stat --oneline --decorate HEAD`: expected OMFS deletion and direct wiring cleanup only.
- `git show --check --oneline HEAD`: clean.
- `git show --name-only --oneline HEAD`: changed files match the selected OMFS feature family.
- Targeted `git show -- Documentation/filesystems/index.rst MAINTAINERS fs/Kconfig fs/Makefile tools/testing/selftests/filesystems/statmount/statmount_test.c ...`: only direct OMFS references removed.
- `rg -n "\bCONFIG_OMFS_FS\b|\bOMFS_FS\b|fs/omfs|\bomfs\b" --glob '!.codex-qemu-kernels/**'`: no remaining source references.
- `git status --short --branch`: clean source tree; branch ahead of origin only.

Review status: clean.
