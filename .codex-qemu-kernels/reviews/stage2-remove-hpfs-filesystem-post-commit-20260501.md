Patch: stage2-remove-hpfs-filesystem
Commit: c0edfea0ae16
Patch class: delete_plus_build_wiring
Reviewer: Codex
Date: 2026-05-01

Scope reviewed:
- Removed HPFS source tree and documentation.
- Removed direct fs/Kconfig and fs/Makefile build wiring.
- Removed MAINTAINERS entry and stale CONFIG_HPFS_FS non-scope defconfig selections.
- Removed the HPFS token from statmount known-filesystem test metadata.

Validation evidence:
- JOBS=8 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh
- x86_64 QEMU Minix NVMe/SCSI log: .codex-qemu-kernels/logs/qemu/minix-storage-x86_64-20260501-191107-attempt1.log
- arm64 QEMU Minix NVMe/SCSI log: .codex-qemu-kernels/logs/qemu/minix-storage-arm64-20260501-191111-attempt1.log
- Metrics: .codex-qemu-kernels/metrics/metrics-20260501-191113.txt

Review commands:
- git show --stat --oneline --decorate HEAD
- git show --check --oneline HEAD
- git show --name-only --oneline HEAD
- git show -- Documentation/filesystems/index.rst MAINTAINERS fs/Kconfig fs/Makefile tools/testing/selftests/filesystems/statmount/statmount_test.c arch/mips/configs/rm200_defconfig arch/powerpc/configs/fsl-emb-nonhw.config arch/m68k/configs/amiga_defconfig arch/m68k/configs/apollo_defconfig arch/m68k/configs/atari_defconfig arch/m68k/configs/bvme6000_defconfig arch/m68k/configs/hp300_defconfig arch/m68k/configs/mac_defconfig arch/m68k/configs/multi_defconfig arch/m68k/configs/mvme147_defconfig arch/m68k/configs/mvme16x_defconfig arch/m68k/configs/q40_defconfig arch/m68k/configs/sun3_defconfig arch/m68k/configs/sun3x_defconfig
- rg -n "CONFIG_HPFS_FS|HPFS_FS|fs/hpfs|\bhpfs\b" --glob '!.codex-qemu-kernels/**'
- git status --short --branch

Findings:
- Clean. The commit removes one complete standalone filesystem family.
- HPFS is outside the fixed validation contract, which keeps Minix plus required VFS/core helpers and QEMU NVMe/SCSI storage paths.
- No exact CONFIG_HPFS_FS, HPFS_FS, fs/hpfs, or hpfs references remain outside .codex-qemu-kernels.
- git show --check reported no whitespace or patch formatting errors.
- Worktree source status is clean after commit; branch is ahead of origin by 9 commits.
