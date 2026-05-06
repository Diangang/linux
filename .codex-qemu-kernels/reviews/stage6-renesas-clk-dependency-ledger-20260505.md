# Stage 6 dependency ledger: Renesas clock-provider deletion

## Assumptions

- Current stage order keeps this round in the concrete `drivers/` broad area.
- Whole directory deletion remains preferred over whole-file or file-internal trimming.
- The selected deletion unit is the Renesas SoC clock provider leaf under `drivers/clk/renesas/`, plus only its direct parent build/config wiring and the dead `include/linux/clk/renesas.h` helper header.
- Renesas dt-binding headers and `include/linux/soc/renesas/*` are not part of this drivers-leaf patch. They either are ABI-like binding-number headers or have other disabled platform driver users, and require a separate include-surface proof pass before removal.

## Candidate size

- `drivers/clk/renesas/`: 62 tracked files, 23595 lines.
- `include/linux/clk/renesas.h`: 1 tracked header, 191 lines.
- Total selected source surface: 63 tracked files, 23786 lines.

## Fixed-config proof

- `.codex-qemu-kernels/build-arm64/.config` has `# CONFIG_ARCH_RENESAS is not set`.
- `arch/arm64/configs/defconfig` has `# CONFIG_ARCH_RENESAS is not set`.
- `.codex-qemu-kernels/build-x86_64/.config` has no `ARCH_RENESAS` or Renesas clock enablement.
- The `drivers/clk/renesas/Kconfig` root symbol `CLK_RENESAS` defaults to `y` only for `ARCH_RENESAS`; fixed x86_64 and arm64 builds do not enable it.

## Build-artifact proof

- `.codex-qemu-kernels/build-x86_64/drivers/clk/renesas/` contains only empty directory aggregate artifacts: `built-in.a`, `modules.order`, `.built-in.a.cmd`, and `.modules.order.cmd`.
- `.codex-qemu-kernels/build-arm64/drivers/clk/renesas/` contains only the same aggregate artifacts.
- `rg` over fixed-build `.cmd` files found no `drivers/clk/renesas`, `clk/renesas`, `renesas-cpg`, `rcar-*`, `rzg2l`, `rzv2h`, `CLK_RENESAS`, `CLK_RCAR`, `CLK_RZG`, or `CLK_RZV` dependency edge.

## Parent wiring

- `drivers/clk/Kconfig:514` sources `drivers/clk/renesas/Kconfig`.
- `drivers/clk/Makefile:125` unconditionally descends into `renesas/`.
- Removing those two direct parent edges is mechanical build/config cleanup for the directory deletion.

## External references

- `rg` outside `drivers/clk/renesas/` found no users of `include/linux/clk/renesas.h` or its helper API declarations/macros.
- `include/linux/soc/renesas/r9a06g032-sysctrl.h` is still included by `drivers/dma/dw/rzn1-dmamux.c`; it is not dead include surface for this patch.
- `include/linux/soc/renesas/rcar-rst.h` has declarations used inside the selected clock directory and fallback stubs for other Renesas platform surfaces; leave it for a separate include/platform proof pass.

## Decision

- Safe target: delete `drivers/clk/renesas/`, remove the two parent `drivers/clk` wiring lines, and delete `include/linux/clk/renesas.h`.
- Patch class: `delete_plus_build_wiring`.
- No runtime logic, control flow, error handling, locking, reference counting, I/O behavior, mount/read/write behavior, syscall/UAPI behavior, or device-name changes are needed.

## Verification plan

1. Confirm `drivers/clk/renesas` and `include/linux/clk/renesas.h` are gone after deletion.
2. Run `git diff --check` and `git diff --cached --check`.
3. Run `JOBS=32 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh`.
4. Commit only kernel source changes.
5. Review with `git show --stat --oneline --decorate HEAD`, `git show --check --oneline HEAD`, and a focused `git show -- drivers/clk/Kconfig drivers/clk/Makefile include/linux/clk/renesas.h`.
