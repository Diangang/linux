# Stage 2 AFFS removal commit blocker

Patch: stage2-remove-affs-filesystem
Patch class: delete_plus_build_wiring
Status: validated, not committed

## Source changes

- Deleted the AFFS filesystem implementation under `fs/affs/`.
- Removed AFFS filesystem Kconfig and Makefile build wiring from `fs/Kconfig` and `fs/Makefile`.
- Removed the stale `AFFS_FS=y` default path from `block/partitions/Kconfig` for Amiga partition parsing.
- Kept `include/uapi/linux/affs_hardblocks.h` because `block/partitions/amiga.c` includes it for partition-format parsing outside the AFFS filesystem implementation.

## Validation

Command:

```
JOBS=8 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh
```

Result: passed.

Evidence:

- x86_64 build completed and QEMU initramfs Minix storage test passed.
- arm64 build completed and QEMU initramfs Minix storage test passed.
- The generated initramfs mounted and exercised QEMU NVMe `/dev/nvme0n1` and SCSI HDD `/dev/sda` Minix filesystems.

## Commit blocker

`git add block/partitions/Kconfig fs/Kconfig fs/Makefile fs/affs` failed:

```
fatal: Unable to create '/data25/lidg/diangang/.git/index.lock': Read-only file system
```

Mount state shows the worktree is writable but `.git` is mounted read-only:

```
/dev/nvme0n1p3 on /data25/lidg/diangang type ext4 (rw,nosuid,nodev,noatime,discard,nobarrier,stripe=32)
/dev/nvme0n1p3 on /data25/lidg/diangang/.git type ext4 (ro,nosuid,nodev,noatime,discard,nobarrier,stripe=32)
```

Allowed stop condition: `codex_process_failed`.
