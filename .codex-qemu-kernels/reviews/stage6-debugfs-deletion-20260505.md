# Stage 6 debugfs deletion review

Commit: 1bda875da9de refactor(裁剪debugfs): 删除固定镜像未用实现

Scope:
- Disabled arm64 fixed config DEBUG_FS and BLK_DEBUG_FS.
- Removed fs/Makefile debugfs build entry.
- Deleted fs/debugfs/ implementation files.
- Kept fs/pstore/ for panic persistence/debuggability.

Dependency ledger:
- x86_64 fixed config already had CONFIG_DEBUG_FS disabled.
- arm64 fixed config now has CONFIG_DEBUG_FS disabled.
- arm64 DEBUG_FS selectors checked before deletion: ZSMALLOC_STAT, DEBUG_CLOSURES, FAIL_FUTEX, KCOV, and BLK_DEV_IO_TRACE are not enabled in the fixed arm64 config.
- BLK_DEBUG_FS depended on DEBUG_FS and was removed from the fixed arm64 defconfig.

Validation:
- Command: JOBS=32 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh
- Result: passed
- x86_64 QEMU log: .codex-qemu-kernels/logs/qemu/minix-storage-x86_64-20260505-185738-attempt1.log
- arm64 QEMU log: .codex-qemu-kernels/logs/qemu/minix-storage-arm64-20260505-185741-attempt1.log
- Metrics: .codex-qemu-kernels/metrics/metrics-20260505-185742.txt
- x86_64 bzImage: 4662272 bytes
- arm64 Image: 10428928 bytes
- Generated configs: CONFIG_DEBUG_FS is disabled for both x86_64 and arm64.

Review:
- git show --check --oneline HEAD: passed.
- Source patch is deletion/config/build-wiring cleanup only; no runtime logic body was changed.
- Residual risk: enabling DEBUG_FS outside the fixed configs would now require restoring or replacing fs/debugfs. This is out of scope for the fixed x86_64/arm64 QEMU storage contract.
