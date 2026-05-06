# Stage 2 Dependency Ledger: ADFS Filesystem

Target: remove the ADFS filesystem implementation under `fs/adfs/`.

Patch class: `delete_plus_build_wiring`.

Contract relevance:

- The fixed validation contract formats and mounts only Minix filesystems.
- Required storage paths are QEMU NVMe `/dev/nvme0n1` and virtio-scsi HDD `/dev/sda`.
- ADFS is an Acorn filesystem and is not required for x86_64 or arm64 Minix mount/write/read/sync/umount validation.

Dependency check:

- `fs/Kconfig` sources `fs/adfs/Kconfig`; this source entry must be removed.
- `fs/Makefile` builds `fs/adfs/` through `CONFIG_ADFS_FS`; this object entry must be removed.
- `block/partitions/acorn.c` includes `include/linux/adfs_fs.h` for Acorn partition table parsing. That shared header is not part of the filesystem module deletion and must remain.
- No x86_64 or arm64 fixed validation device path depends on `CONFIG_ADFS_FS`.

Decision:

- Safe to delete `fs/adfs/` and remove its `fs/Kconfig` and `fs/Makefile` wiring.
- Do not remove `include/linux/adfs_fs.h` or `include/uapi/linux/adfs_fs.h` in this patch because they are still used by Acorn partition parsing.
