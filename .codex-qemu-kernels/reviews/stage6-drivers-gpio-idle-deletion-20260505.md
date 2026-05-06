# Stage 6 drivers gpio/idle deletion review

Commit: eb39b20a24b9 refactor(裁剪drivers): 删除未用gpio和idle入口

Scope:
- Deleted drivers/gpio/Kconfig and removed its drivers/Kconfig source entry.
- Deleted drivers/idle/ files and removed drivers/Makefile plus arch/x86/Kconfig entries.
- Removed fixed defconfig GPIOLIB_LEGACY residue for x86_64 and arm64.
- Removed physical empty directories fs/debugfs, drivers/gpio, and drivers/idle from the worktree; Git records only file deletions.

Dependency ledger:
- Fixed x86_64 and arm64 builds had no drivers/gpio objects.
- Fixed x86_64 and arm64 builds had only empty drivers/idle built-in archives, no intel_idle object.
- Generated fixed configs after validation have no GPIOLIB, GPIOLIB_LEGACY, CPU_IDLE, or INTEL_IDLE entries.
- No fixed-build source callers of legacy gpio_request/gpio_free/gpio_direction/gpio_get/gpio_set APIs were found outside the deleted GPIO Kconfig surface.

Validation:
- Command: JOBS=32 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh
- Result: passed
- x86_64 QEMU log: .codex-qemu-kernels/logs/qemu/minix-storage-x86_64-20260505-190109-attempt1.log
- arm64 QEMU log: .codex-qemu-kernels/logs/qemu/minix-storage-arm64-20260505-190112-attempt1.log
- Metrics: .codex-qemu-kernels/metrics/metrics-20260505-190113.txt
- x86_64 bzImage: 4662272 bytes
- arm64 Image: 10428928 bytes

Review:
- git show --check --oneline HEAD: passed.
- Source patch is deletion/config/build-wiring cleanup only; no runtime logic body was changed.
- pstore remains kept for panic persistence/debuggability.
