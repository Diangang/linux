# Stage 2 BFS dependency ledger

Target: remove the SCO UnixWare BFS filesystem implementation.

Patch class: delete_plus_build_wiring.

Contract check:
- Fixed filesystem requirement is Minix only, mounted on `/dev/nvme0n1` and `/dev/sda`.
- BFS is selected only by `CONFIG_BFS_FS` and builds only `fs/bfs/`.
- BFS Kconfig describes a SCO UnixWare `/stand` filesystem and UnixWare slice workflow, not the QEMU NVMe/SCSI Minix path.
- No x86_64 or arm64 fixed validation config requires `CONFIG_BFS_FS`.
- The storage keep list remains NVMe, SCSI core, SCSI disk, virtio-scsi, Minix, initramfs, devtmpfs, and required QEMU PCI/interrupt paths.

Dependency search:
- `fs/Kconfig` sources `fs/bfs/Kconfig` only under miscellaneous filesystems.
- `fs/Makefile` includes `fs/bfs/` only for `CONFIG_BFS_FS`.
- Stale `CONFIG_BFS_FS=m` selections exist only in out-of-scope mips/powerpc defconfigs.
- `include/uapi/linux/bfs_fs.h` is not removed in this patch because deleting exported UAPI headers is outside the deletion policy for this round.

Decision:
- Safe to delete `fs/bfs/` and remove its build/Kconfig and stale defconfig selections.
- No runtime logic, control flow, error handling, locking, reference counting, I/O behavior, mount/read/write behavior, syscall/UAPI behavior, or device names are changed.
