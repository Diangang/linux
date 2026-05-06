# Stage 1 Fixed Storage Validation

Command:

```
JOBS=8 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh
```

Result: pass.

Evidence:

- x86_64 kernel built at `.codex-qemu-kernels/build-x86_64/arch/x86/boot/bzImage`.
- arm64 kernel built at `.codex-qemu-kernels/build-arm64/arch/arm64/boot/Image`.
- x86_64 QEMU log: `.codex-qemu-kernels/logs/qemu/minix-storage-x86_64-20260430-222945-attempt1.log`.
- arm64 QEMU log: `.codex-qemu-kernels/logs/qemu/minix-storage-arm64-20260430-222949-attempt1.log`.
- Both QEMU logs contain `CODEX_MINIX_TEST_PASS`.
- QEMU stderr files for both runs are empty.
- Metrics: `.codex-qemu-kernels/metrics/metrics-20260430-222951.txt`.

Contract checked:

- Generated initramfs with `rdinit=/init`.
- Raw disposable NVMe and SCSI HDD images formatted with `mkfs.minix -3`.
- Guest exercised `/dev/nvme0n1` and `/dev/sda` by mounting, writing, reading, syncing, and unmounting Minix filesystems.
- No Debian qcow2 root filesystem and no virtio-blk root-device dependency.

Review: clean. Stage 1 automation is usable as the fixed validation gate for source deletion patches.
