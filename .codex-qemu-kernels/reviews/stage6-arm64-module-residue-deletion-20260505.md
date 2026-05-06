# Stage 6 arm64 module residue deletion review

Commit: 0527da1f93e8 refactor(裁剪arm64): 删除非存储模块残留

Scope:
- Deleted PCI pwrctrl module files and stale PCI Kconfig/Makefile references.
- Deleted Google firmware module directory and stale firmware Kconfig/Makefile references.
- Deleted Xilinx and DesignWare DMA module files and stale DMA Kconfig/Makefile references.
- Deleted Xilinx clock module files and stale clock Kconfig/Makefile references.
- Removed matching arm64 defconfig module requests and x86_64 stale disabled entries.
- Kept pstore, pstore_ram, and reed_solomon.

Validation:
- Command: JOBS=32 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh
- Result: passed
- x86_64 QEMU log: .codex-qemu-kernels/logs/qemu/minix-storage-x86_64-20260505-190455-attempt1.log
- arm64 QEMU log: .codex-qemu-kernels/logs/qemu/minix-storage-arm64-20260505-190458-attempt1.log
- Metrics: .codex-qemu-kernels/metrics/metrics-20260505-190459.txt
- x86_64 bzImage: 4662272 bytes
- arm64 Image: 10428928 bytes
- arm64 module configs after validation: PSTORE_RAM and REED_SOLOMON only.

Review:
- git show --check --oneline HEAD: passed.
- Source patch is deletion/config/build-wiring cleanup only; no runtime logic body was changed.
- Physical empty directories for the deleted module families were removed from the worktree.
