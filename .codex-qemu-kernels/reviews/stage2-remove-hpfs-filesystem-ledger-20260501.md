Patch: stage2-remove-hpfs-filesystem
Patch class: delete_plus_build_wiring
Target: HPFS filesystem, CONFIG_HPFS_FS, fs/hpfs/
Date: 2026-05-01

Assumptions:
- The fixed contract requires Minix plus VFS/core helpers, block layer, NVMe, SCSI core, SCSI disk, virtio-scsi, PCI/interrupt support, devtmpfs, proc/sysfs, and generated initramfs boot paths.
- HPFS is a standalone OS/2 filesystem and is not used by the generated initramfs Minix NVMe/SCSI validation.
- Removing HPFS must not alter mount/read/write/sync/umount semantics for Minix or storage device names.

Dependency evidence:
- fs/hpfs/Kconfig defines CONFIG_HPFS_FS as a tristate filesystem depending on BLOCK, selecting BUFFER_HEAD and FS_IOMAP.
- fs/hpfs/Makefile builds hpfs.o only under CONFIG_HPFS_FS from HPFS-local objects.
- fs/Kconfig sources fs/hpfs/Kconfig.
- fs/Makefile descends into fs/hpfs/ only under CONFIG_HPFS_FS.
- Documentation/filesystems/index.rst and Documentation/filesystems/hpfs.rst are HPFS documentation.
- MAINTAINERS has an HPFS FILESYSTEM block with fs/hpfs/.
- tools/testing/selftests/filesystems/statmount/statmount_test.c has an HPFS known-filesystem token; it is direct metadata for a removed filesystem.
- Stale CONFIG_HPFS_FS=m selections exist only in non-scope MIPS, m68k, and PowerPC defconfigs found by exact reference search.

Files planned:
- Delete Documentation/filesystems/hpfs.rst.
- Delete fs/hpfs/ implementation files.
- Remove HPFS entries from Documentation/filesystems/index.rst, MAINTAINERS, fs/Kconfig, fs/Makefile, statmount known_fs, and stale non-scope defconfigs.

Verification plan:
- rg -n "CONFIG_HPFS_FS|HPFS_FS|fs/hpfs|\bhpfs\b" --glob '!.codex-qemu-kernels/**'
- git diff --check
- JOBS=8 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh
- If validation passes: commit source only, then run git show --stat, git show --check, targeted git show, exact-reference rg, and git status.
