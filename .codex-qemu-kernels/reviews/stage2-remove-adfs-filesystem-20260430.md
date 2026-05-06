# Stage 2 Review: Remove ADFS Filesystem

Patch: `stage2-remove-adfs-filesystem`

Scope:

- Removed the complete ADFS filesystem implementation under `fs/adfs/`.
- Removed its source entry from `fs/Kconfig`.
- Removed its object entry from `fs/Makefile`.
- Kept `include/linux/adfs_fs.h` and `include/uapi/linux/adfs_fs.h` because Acorn partition parsing still uses the shared definitions.

Patch class: `delete_plus_build_wiring`

Contract relevance:

- The fixed validation contract uses only Minix filesystems on QEMU NVMe and SCSI HDD devices.
- ADFS is outside the retained storage/filesystem surface.
- No runtime logic was changed and no stubs were introduced.

Validation command:

```sh
JOBS=8 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh
```

Result: pass.

Evidence:

- x86_64 QEMU log: `.codex-qemu-kernels/logs/qemu/minix-storage-x86_64-20260430-225421-attempt1.log`
- arm64 QEMU log: `.codex-qemu-kernels/logs/qemu/minix-storage-arm64-20260430-225426-attempt1.log`
- Metrics: `.codex-qemu-kernels/metrics/metrics-20260430-225428.txt`
- Both QEMU logs contain `CODEX_MINIX_TEST_PASS`.

Review: clean. Commit may include only the kernel source deletion and build/config wiring cleanup.
