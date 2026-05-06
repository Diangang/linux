# Stage 6 dependency ledger: i.MX clock-provider deletion

## Assumptions

- Current stage order keeps this round in the concrete `drivers/` broad area.
- Whole directory deletion remains preferred over whole-file or file-internal trimming.
- The selected deletion unit is the NXP/Freescale i.MX clock provider leaf under `drivers/clk/imx/`, plus only its direct parent build/config wiring and the dead `include/linux/clk/imx.h` helper header.
- i.MX firmware, SoC, platform-data, media, dt-binding, and MFD headers are not part of this drivers-leaf patch. They have separate disabled platform surfaces and need a separate include/platform proof pass before removal.

## Candidate size

- `drivers/clk/imx/`: 60 tracked files, 20347 lines.
- `include/linux/clk/imx.h`: 1 tracked header, 14 lines.
- Total selected source surface: 61 tracked files, 20361 lines.

## Fixed-config proof

- `rg` over `.codex-qemu-kernels/build-x86_64/.config`, `.codex-qemu-kernels/build-arm64/.config`, and `arch/arm64/configs/defconfig` found no `CONFIG_ARCH_MXC`, `CONFIG_MXC_CLK`, `CONFIG_CLK_IMX*`, `CONFIG_SOC_IMX*`, `CONFIG_SOC_VF610`, `CONFIG_IMX_SCU`, or `CONFIG_SOC_IMXRT` enablement.
- The `drivers/clk/imx/Kconfig` root symbol `MXC_CLK` depends on `ARCH_MXC || COMPILE_TEST`; fixed x86_64 and arm64 builds do not enable it.

## Build-artifact proof

- `.codex-qemu-kernels/build-x86_64/drivers/clk/imx/` contains only empty directory aggregate artifacts: `built-in.a`, `modules.order`, `.built-in.a.cmd`, and `.modules.order.cmd`.
- `.codex-qemu-kernels/build-arm64/drivers/clk/imx/` contains only the same aggregate artifacts.
- `rg` over fixed-build `.cmd` files found no `drivers/clk/imx`, `clk/imx`, `mxc-clk`, `clk-imx`, `CLK_IMX`, `MXC_CLK`, `SOC_IMX`, or `ARCH_MXC` dependency edge.

## Parent wiring

- `drivers/clk/Kconfig:507` sources `drivers/clk/imx/Kconfig`.
- `drivers/clk/Makefile:118` unconditionally descends into `imx/`.
- Removing those two direct parent edges is mechanical build/config cleanup for the directory deletion.

## External references

- `include/linux/clk/imx.h` declares only `imx6sl_set_wait_clk(bool enter)`.
- `rg` outside `drivers/clk/imx/` and outside the header found no real users of `include/linux/clk/imx.h` or `imx6sl_set_wait_clk()`. The only unrelated match was a local variable name in `drivers/clk/actions/owl-s900.c`.
- i.MX firmware and SoC headers are not dead include surface for this patch and are intentionally left in place.

## Decision

- Safe target: delete `drivers/clk/imx/`, remove the two parent `drivers/clk` wiring lines, and delete `include/linux/clk/imx.h`.
- Patch class: `delete_plus_build_wiring`.
- No runtime logic, control flow, error handling, locking, reference counting, I/O behavior, mount/read/write behavior, syscall/UAPI behavior, or device-name changes are needed.

## Verification plan

1. Confirm `drivers/clk/imx` and `include/linux/clk/imx.h` are gone after deletion.
2. Run `git diff --check` and `git diff --cached --check`.
3. Run `JOBS=32 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh`.
4. Commit only kernel source changes.
5. Review with `git show --stat --oneline --decorate HEAD`, `git show --check --oneline HEAD`, and a focused `git show -- drivers/clk/Kconfig drivers/clk/Makefile include/linux/clk/imx.h`.
