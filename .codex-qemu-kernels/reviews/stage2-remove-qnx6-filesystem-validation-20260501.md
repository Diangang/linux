# stage2-remove-qnx6-filesystem validation

Patch: remove QNX6 filesystem implementation, build wiring, documentation,
private `include/linux/qnx6_fs.h` header, MAINTAINERS block, and stale m68k
defconfig selections.

Validation command:

```sh
JOBS=8 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh
```

Result: passed.

Evidence:

- x86_64 QEMU Minix storage test passed on attempt 1:
  `.codex-qemu-kernels/logs/qemu/minix-storage-x86_64-20260501-174913-attempt1.log`
- arm64 QEMU Minix storage test passed on attempt 1:
  `.codex-qemu-kernels/logs/qemu/minix-storage-arm64-20260501-174917-attempt1.log`
- Metrics:
  `.codex-qemu-kernels/metrics/metrics-20260501-174918.txt`

Scope notes:

- QNX6 is outside the fixed validation contract, which keeps Minix plus
  required VFS/core helpers and QEMU NVMe/SCSI storage paths.
- The generic UAPI magic value remains outside this deletion patch; only the
  private QNX6 filesystem header is removed.
- `git diff --check` passed before commit.
