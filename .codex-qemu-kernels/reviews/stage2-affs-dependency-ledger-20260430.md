# Stage 2 dependency ledger: AFFS filesystem

Target: remove the Amiga Fast File System implementation (`fs/affs/`).

Patch class: `delete_plus_build_wiring`.

Contract check:
- Fixed filesystem contract uses only Minix (`CONFIG_MINIX_FS`) for `/dev/nvme0n1` and `/dev/sda`.
- Fixed storage contract keeps NVMe, SCSI core, SCSI disk, virtio-scsi, initramfs, devtmpfs, PCI/interrupt support.
- AFFS is not mounted, formatted, or probed by the initramfs storage test.

Dependency scan:
- `fs/Kconfig` sources `fs/affs/Kconfig`; this is AFFS-only build/config surface.
- `fs/Makefile` builds `fs/affs/` only through `CONFIG_AFFS_FS`.
- `block/partitions/Kconfig` has an Amiga partition default tied to `AFFS_FS=y`; after deleting `CONFIG_AFFS_FS`, that default is dead config wiring and can be removed while keeping `AMIGA` behavior intact.
- `block/partitions/amiga.c` includes `<linux/affs_hardblocks.h>`; keep `include/uapi/linux/affs_hardblocks.h` because it is partition parsing format data, not the AFFS filesystem implementation.
- Other `AFFS_FS` references found are out-of-scope architecture defconfigs or documentation/metadata and do not affect x86_64 or arm64 validation builds.

Decision:
- Safe deletion target under the fixed validation contract.
- Delete `fs/affs/`.
- Remove AFFS entries from `fs/Kconfig`, `fs/Makefile`, and the dead `AFFS_FS=y` part of the Amiga partition default.
- No runtime logic, I/O behavior, syscall/UAPI, mount behavior, locking, reference counting, or device naming changes.
