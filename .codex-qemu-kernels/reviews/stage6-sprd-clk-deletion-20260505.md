# Stage 6 review: Spreadtrum clock provider deletion

Target: `drivers/clk/sprd/`

Commit: `6e953cfc9d81 refactor(裁剪clk): 删除Spreadtrum时钟驱动`

Patch class: `delete_plus_build_wiring`

Dependency ledger: `.codex-qemu-kernels/reviews/stage6-sprd-clk-dependency-ledger-20260505.md`

Findings:

- The commit deletes the Spreadtrum/Sprd SC986x/UMS512 clock-provider directory and only the direct `drivers/clk/Kconfig` source edge and `drivers/clk/Makefile` `obj-y` edge.
- Fixed arm64 config has `# CONFIG_ARCH_SPRD is not set`; fixed x86_64/arm64 configs do not enable `SPRD_COMMON_CLK` or `SPRD_*_CLK`.
- Generated `.cmd` dependency scans found no fixed build edge to `drivers/clk/sprd/`, `clk/sprd`, `clk-sprd`, `sc986*`, `ums512`, `SPRD_COMMON_CLK`, `SPRD_*_CLK`, or `ARCH_SPRD`.
- Remaining Sprd references are arm64 platform Kconfig, dt-binding headers, and independent clocksource/DMA/IOMMU/serial platform residue intentionally left for separate proof passes.
- Required validation passed with `JOBS=32 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh`: x86_64 and arm64 built and completed initramfs QEMU Minix storage tests on `/dev/nvme0n1` and `/dev/sda` on attempt 1.
- Metrics: x86_64 `bzImage` 4662272 bytes, arm64 `Image` 10428928 bytes; enabled config counts remain x86_64=876 and arm64=868.
- Physical empty source directory `drivers/clk/sprd/` is gone from the worktree.
- `git show --check --oneline HEAD` reported no whitespace errors.

Review result: clean.
