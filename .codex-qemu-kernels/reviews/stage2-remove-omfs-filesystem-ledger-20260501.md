Patch: stage2-remove-omfs-filesystem
Patch class: delete_plus_build_wiring
Target: OMFS filesystem, CONFIG_OMFS_FS, fs/omfs/
Date: 2026-05-01

Assumptions:
- The fixed contract requires Minix plus VFS/core helpers, block layer, NVMe, SCSI core, SCSI disk, virtio-scsi, PCI/interrupt support, devtmpfs, proc/sysfs, and generated initramfs boot paths.
- OMFS is a standalone SonicBlue/Rio Karma/ReplayTV filesystem and is not used by generated initramfs Minix NVMe/SCSI validation.
- Removing OMFS must not alter mount/read/write/sync/umount semantics for Minix or storage device names.

Dependency evidence:
- fs/omfs/Kconfig defines CONFIG_OMFS_FS as a tristate filesystem depending on BLOCK and selecting BUFFER_HEAD and CRC_ITU_T.
- fs/omfs/Makefile builds omfs.o only under CONFIG_OMFS_FS from OMFS-local objects.
- fs/Kconfig sources fs/omfs/Kconfig.
- fs/Makefile descends into fs/omfs/ only under CONFIG_OMFS_FS.
- Documentation/filesystems/index.rst and Documentation/filesystems/omfs.rst are OMFS documentation.
- MAINTAINERS has an OMFS FILESYSTEM block with fs/omfs/.
- tools/testing/selftests/filesystems/statmount/statmount_test.c has an OMFS known-filesystem token; it is direct metadata for a removed filesystem.
- Stale CONFIG_OMFS_FS=m selections exist only in non-scope m68k, MIPS, and PowerPC defconfigs found by exact reference search.

Files planned:
- Delete Documentation/filesystems/omfs.rst.
- Delete fs/omfs/ implementation files.
- Remove OMFS entries from Documentation/filesystems/index.rst, MAINTAINERS, fs/Kconfig, fs/Makefile, statmount known_fs, and stale non-scope defconfigs.

Verification plan:
- rg -n "\bCONFIG_OMFS_FS\b|\bOMFS_FS\b|fs/omfs|\bomfs\b" --glob '!.codex-qemu-kernels/**'
- git diff --check
- JOBS=8 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh
- If validation passes: commit source only, then run git show --stat, git show --check, targeted git show, exact-reference rg, and git status.
