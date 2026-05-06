# Stage 6 EFI STMM deletion review

Commit: `fb96a84b588c` (`refactor(裁剪efi): 删除TEE STMM变量服务`)

Patch class: `delete_plus_build_wiring`

Scope:

- Removed `drivers/firmware/efi/stmm/mm_communication.h`.
- Removed `drivers/firmware/efi/stmm/tee_stmm_efi.c`.
- Removed the dead `TEE_STMM_EFI` Kconfig entry.
- Removed the dead `obj-$(CONFIG_TEE_STMM_EFI)` Makefile edge.

Validation:

- `JOBS=32 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh`
  passed.
- x86_64 QEMU Minix storage test passed on attempt 1:
  `.codex-qemu-kernels/logs/qemu/minix-storage-x86_64-20260505-192749-attempt1.log`
- arm64 QEMU Minix storage test passed on attempt 1:
  `.codex-qemu-kernels/logs/qemu/minix-storage-arm64-20260505-192752-attempt1.log`
- Metrics: `.codex-qemu-kernels/metrics/metrics-20260505-192753.txt`
  recorded x86_64 `bzImage` size 4662272 bytes, arm64 `Image` size 10428928
  bytes, enabled config counts x86_64=876 and arm64=868.

Review:

- `git show --stat --oneline --decorate HEAD` shows only four EFI STMM files
  changed, with 849 deletions.
- `git show --check --oneline HEAD` reported no whitespace errors.
- `git show -- drivers/firmware/efi` confirmed the patch only deletes the STMM
  Kconfig entry, Makefile object edge, and the two STMM implementation files.
- The physical `drivers/firmware/efi/stmm/` directory was removed from the
  working tree.

Findings:

- No review findings.
- No runtime EFI logic, storage path, syscall/UAPI behavior, locking, error
  handling, device names, or mount/read/write behavior was changed.
