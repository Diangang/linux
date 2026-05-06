Patch: stage2-remove-befs-filesystem
Commit: 75267a547963
Patch class: delete_plus_build_wiring
Reviewer: Codex
Date: 2026-05-01

Scope reviewed:
- Removed BeFS source tree and documentation.
- Removed direct fs/Kconfig and fs/Makefile build wiring.
- Removed MAINTAINERS entry and stale CONFIG_BEFS_FS non-scope defconfig selections.
- Removed the BeFS token from statmount known-filesystem test metadata.

Validation evidence:
- JOBS=8 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh
- x86_64 QEMU Minix NVMe/SCSI log: .codex-qemu-kernels/logs/qemu/minix-storage-x86_64-20260501-185328-attempt1.log
- arm64 QEMU Minix NVMe/SCSI log: .codex-qemu-kernels/logs/qemu/minix-storage-arm64-20260501-185332-attempt1.log
- Metrics: .codex-qemu-kernels/metrics/metrics-20260501-185334.txt

Review commands:
- git show --stat --oneline --decorate HEAD
- git show --check --oneline HEAD
- git show --name-only --oneline HEAD
- git show -- Documentation/filesystems/index.rst MAINTAINERS fs/Kconfig fs/Makefile tools/testing/selftests/filesystems/statmount/statmount_test.c arch/mips/configs/malta_defconfig arch/mips/configs/rm200_defconfig arch/mips/configs/malta_kvm_defconfig arch/mips/configs/maltaup_xpa_defconfig arch/powerpc/configs/fsl-emb-nonhw.config arch/powerpc/configs/ppc6xx_defconfig
- rg -n "CONFIG_BEFS_FS|BEFS_FS|fs/befs|\bbefs\b" --glob '!.codex-qemu-kernels/**'
- git status --short --branch

Findings:
- Clean. The commit removes one complete standalone filesystem family.
- BeFS is outside the fixed validation contract, which keeps Minix plus required VFS/core helpers and QEMU NVMe/SCSI storage paths.
- No exact CONFIG_BEFS_FS, BEFS_FS, fs/befs, or befs references remain outside .codex-qemu-kernels.
- git show --check reported no whitespace or patch formatting errors.
- Worktree source status is clean after commit; branch is ahead of origin by 8 commits.
