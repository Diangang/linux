# Stage 6 review: Broadcom clock provider deletion

Target: `drivers/clk/bcm/`

Commit: `8dc47a76fbcc refactor(裁剪clk): 删除Broadcom时钟驱动`

Patch class: `delete_plus_build_wiring`

Dependency ledger: `.codex-qemu-kernels/reviews/stage6-bcm-clk-dependency-ledger-20260505.md`

Findings:

- The commit deletes the Broadcom BCM/Raspberry Pi/iProc clock-provider directory and only the direct `drivers/clk/Kconfig` source edge and `drivers/clk/Makefile` `obj-y` edge.
- Fixed arm64 config has `# CONFIG_ARCH_BCM is not set`; fixed x86_64/arm64 configs do not enable `CLK_BCM*`, `COMMON_CLK_IPROC`, `CLK_RASPBERRYPI`, or `RASPBERRYPI_FIRMWARE`.
- Generated `.cmd` dependency scans found no fixed build edge to `drivers/clk/bcm/`, `clk/bcm`, `clk-bcm`, `clk-kona`, `clk-iproc`, `clk-raspberrypi`, Broadcom clock symbols, or Raspberry Pi firmware clock symbols.
- No `include/linux/clk/bcm*.h` header exists; the Raspberry Pi firmware platform-device string is not a fixed-config dependency and was intentionally left for a separate firmware/platform proof pass.
- Required validation passed with `JOBS=32 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh`: x86_64 and arm64 built and completed initramfs QEMU Minix storage tests on `/dev/nvme0n1` and `/dev/sda` on attempt 1.
- Metrics: x86_64 `bzImage` 4662272 bytes, arm64 `Image` 10428928 bytes; enabled config counts remain x86_64=876 and arm64=868.
- Physical empty source directory `drivers/clk/bcm/` is gone from the worktree.
- `git show --check --oneline HEAD` reported no whitespace errors.

Review result: clean.
