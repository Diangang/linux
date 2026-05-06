# Stage 6 review: i.MX clock provider deletion

Target: `drivers/clk/imx/` and `include/linux/clk/imx.h`

Commit: `3b1e02fd3cd5 refactor(裁剪clk): 删除i.MX时钟驱动`

Patch class: `delete_plus_build_wiring`

Dependency ledger: `.codex-qemu-kernels/reviews/stage6-imx-clk-dependency-ledger-20260505.md`

Findings:

- The commit deletes the NXP/Freescale i.MX clock-provider directory and only the direct `drivers/clk/Kconfig` source edge, `drivers/clk/Makefile` `obj-y` edge, and dead `include/linux/clk/imx.h` helper declaration header.
- Fixed x86_64 and arm64 configs do not enable `ARCH_MXC`, `MXC_CLK`, `CLK_IMX*`, `SOC_IMX*`, `SOC_VF610`, `IMX_SCU`, or `SOC_IMXRT`.
- Generated `.cmd` dependency scans found no fixed build edge to `drivers/clk/imx/`, `clk/imx`, `mxc-clk`, `clk-imx`, `CLK_IMX`, `MXC_CLK`, `SOC_IMX`, or `ARCH_MXC`; pre-delete build dirs only had empty aggregate artifacts.
- `include/linux/clk/imx.h` had no outside callers; related i.MX firmware, SoC, media, MFD, platform-data, and dt-binding headers were intentionally left for separate proof passes.
- Required validation passed with `JOBS=32 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh`: x86_64 and arm64 built and completed initramfs QEMU Minix storage tests on `/dev/nvme0n1` and `/dev/sda` on attempt 1.
- Metrics: x86_64 `bzImage` 4662272 bytes, arm64 `Image` 10428928 bytes; enabled config counts remain x86_64=876 and arm64=868.
- Physical empty source directory `drivers/clk/imx/` is gone from the worktree.
- `git show --check --oneline HEAD` reported no whitespace errors.

Review result: clean.
