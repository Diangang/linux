# Stage 3 drivers subdirectory deletion pass review

Date: 2026-05-02

Patch class: delete_plus_build_wiring

Scope:

- Removed unused block driver subdirectories: `aoe`, `drbd`, `mtip32xx`,
  `null_blk`, `rnbd`, `rnull`, `xen-blkback`, and `zram`.
- Removed unused character driver subdirectories: `agp`, `ipmi`, `tpm`,
  `xilinx_hwicap`, and `xillybus`.
- Removed NVMe target and common auth KUnit tests while keeping NVMe host.
- Removed OF runtime unittest data and now-unbuilt `drivers/of/unittest.c`.
- Removed legacy ISA PnP and PNPBIOS protocol subdirectories.

Mechanical fallout:

- Removed stale Kconfig `source` entries and Makefile `obj-*` entries from
  kept parent directories.
- Removed `CONFIG_TCG_TPM=y` from `arch/arm64/configs/defconfig`.
- Removed `CONFIG_XEN_BLKDEV_BACKEND=m` from `kernel/configs/xen.config`.

Validation:

- `git diff --check`: passed before validation.
- `JOBS=32 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh`:
  passed.
- x86_64 QEMU Minix storage log:
  `.codex-qemu-kernels/logs/qemu/minix-storage-x86_64-20260502-204645-attempt1.log`
- arm64 QEMU Minix storage log:
  `.codex-qemu-kernels/logs/qemu/minix-storage-arm64-20260502-204648-attempt1.log`
- Metrics: `.codex-qemu-kernels/metrics/metrics-20260502-204649.txt`

Findings:

- No validation regression found for the fixed NVMe plus SCSI HDD Minix
  contract.
- The only non-config source modifications are mechanical Kconfig/Makefile
  fallout and whole-file deletion of the OF runtime unittest source after its
  data directory was removed.
- Temporary/non-config fallout is recorded in the compatibility ledger for
  later rollback review.
