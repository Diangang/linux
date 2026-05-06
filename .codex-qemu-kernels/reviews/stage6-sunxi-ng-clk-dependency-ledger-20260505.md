# Stage 6 dependency ledger: sunxi-ng clock-provider deletion

## Assumptions

- Current stage order keeps this round in the concrete `drivers/` broad area.
- Whole directory deletion remains preferred over whole-file or file-internal trimming.
- The selected deletion unit is the Allwinner/Sunxi common clock provider leaf under `drivers/clk/sunxi-ng/`, plus only its direct parent build/config wiring and the dead public helper header that declares functions implemented only by this leaf.
- Device-tree binding headers under `include/dt-bindings/{clock,reset}/sun*-ccu.h` are not part of this drivers-leaf patch; they are ABI-like binding-number headers and need a separate include-surface proof pass if removed later.

## Candidate size

- `drivers/clk/sunxi-ng/`: 82 tracked files, 27865 lines.
- `include/linux/clk/sunxi-ng.h`: 1 tracked header, 14 lines.
- Total selected source surface: 83 tracked files, 27879 lines.

## Fixed-config proof

- `.codex-qemu-kernels/build-arm64/.config` has `# CONFIG_ARCH_SUNXI is not set`.
- `arch/arm64/configs/defconfig` has `# CONFIG_ARCH_SUNXI is not set`.
- `.codex-qemu-kernels/build-x86_64/.config` has no `ARCH_SUNXI` or `SUNXI_CCU` enablement.
- The `drivers/clk/sunxi-ng/Kconfig` root symbol `SUNXI_CCU` depends on `ARCH_SUNXI || COMPILE_TEST` and defaults to `ARCH_SUNXI`; fixed x86_64 and arm64 builds do not enable it.

## Build-artifact proof

- `.codex-qemu-kernels/build-x86_64/drivers/clk/sunxi-ng/` contains only empty directory aggregate artifacts: `built-in.a`, `modules.order`, `.built-in.a.cmd`, and `.modules.order.cmd`.
- `.codex-qemu-kernels/build-arm64/drivers/clk/sunxi-ng/` contains only the same aggregate artifacts.
- `rg` over fixed-build `.cmd` files found no `drivers/clk/sunxi-ng`, `sunxi-ng`, `sunxi-ccu`, `ccu-sun`, `SUNXI_CCU`, or `include/linux/clk/sunxi-ng.h` dependency edge.

## Parent wiring

- `drivers/clk/Kconfig:520` sources `drivers/clk/sunxi-ng/Kconfig`.
- `drivers/clk/Makefile:131` unconditionally descends into `sunxi-ng/`.
- Removing those two direct parent edges is mechanical build/config cleanup for the directory deletion.

## External references

- `rg` outside `drivers/clk/sunxi-ng/` found no external users of `sunxi_ccu_set_mmc_timing_mode()` or `sunxi_ccu_get_mmc_timing_mode()`.
- The only remaining declaration site for those helpers is `include/linux/clk/sunxi-ng.h`, so that header is dead include surface for this selected feature and should be deleted with the directory.
- Other `ARCH_SUNXI` references remain in unrelated disabled platform Kconfig or driver areas and do not reference this clock-provider leaf directly.

## Decision

- Safe target: delete `drivers/clk/sunxi-ng/`, remove the two parent `drivers/clk` wiring lines, and delete `include/linux/clk/sunxi-ng.h`.
- Patch class: `delete_plus_build_wiring`.
- No runtime logic, control flow, error handling, locking, reference counting, I/O behavior, mount/read/write behavior, syscall/UAPI behavior, or device-name changes are needed.

## Verification plan

1. Confirm `drivers/clk/sunxi-ng` and `include/linux/clk/sunxi-ng.h` are gone after deletion.
2. Run `git diff --check` and `git diff --cached --check`.
3. Run `JOBS=32 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh`.
4. Commit only kernel source changes.
5. Review with `git show --stat --oneline --decorate HEAD`, `git show --check --oneline HEAD`, and a focused `git show -- drivers/clk/Kconfig drivers/clk/Makefile include/linux/clk/sunxi-ng.h`.
