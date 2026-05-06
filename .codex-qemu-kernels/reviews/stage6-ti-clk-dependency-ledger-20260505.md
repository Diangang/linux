# Stage 6 dependency ledger: TI clock provider deletion

Candidate: `drivers/clk/ti/` and `include/linux/clk/ti.h`

Selection assumptions:

- The fixed validation contract is limited to x86_64 and arm64 QEMU boot plus NVMe `/dev/nvme0n1` and SCSI `/dev/sda` Minix mount/write/read/sync/umount.
- TI OMAP/AM/DRA clock-provider support is platform clock initialization, not part of the QEMU storage contract.
- Directory-level deletion remains available in `drivers/`, so file-internal trimming is deferred.

Coarse size check:

- `drivers/clk/ti/`: 32 files, 12790 lines, largest remaining `drivers/clk` leaf after the i.MX deletion.

Build/config wiring:

- Parent config edge: `drivers/clk/Kconfig` sources `drivers/clk/ti/Kconfig`.
- Parent build edge: `drivers/clk/Makefile` adds `obj-y += ti/`.
- `drivers/clk/ti/Kconfig` only defines `COMMON_CLK_TI_ADPLL`, depending on `ARCH_OMAP2PLUS || COMPILE_TEST`.
- `drivers/clk/ti/Makefile` builds most objects only under `CONFIG_ARCH_OMAP2PLUS=y`, and `adpll.o` only under `CONFIG_COMMON_CLK_TI_ADPLL`.

Fixed-config proof:

- Fixed x86_64 `.config`, fixed arm64 `.config`, and `arch/arm64/configs/defconfig` have no enablement for `ARCH_OMAP2PLUS`, `SOC_AM33XX`, `SOC_AM43XX`, `SOC_TI81XX`, `ARCH_OMAP2`, `ARCH_OMAP3`, `ARCH_OMAP4`, `SOC_OMAP5`, `SOC_DRA7XX`, or `COMMON_CLK_TI_ADPLL`.
- Arm64 fixed config explicitly has `# CONFIG_ARCH_K3 is not set`; K3-related non-clock TI surfaces are not a dependency for this directory deletion.

Generated dependency proof:

- `.cmd` scans under `.codex-qemu-kernels/build-x86_64` and `.codex-qemu-kernels/build-arm64` found no dependency edge for `drivers/clk/ti`, `clk/ti`, `clk-ti`, `clkctrl`, `COMMON_CLK_TI`, `COMMON_CLK_TI_ADPLL`, `ARCH_OMAP2PLUS`, `SOC_AM33XX`, `SOC_TI81XX`, `SOC_DRA7XX`, `SOC_OMAP5`, or `SOC_AM43XX`.
- Pre-delete fixed build directories contain only empty aggregate artifacts:
  - `.codex-qemu-kernels/build-x86_64/drivers/clk/ti/{built-in.a,modules.order,.built-in.a.cmd,.modules.order.cmd}`
  - `.codex-qemu-kernels/build-arm64/drivers/clk/ti/{built-in.a,modules.order,.built-in.a.cmd,.modules.order.cmd}`

External reference proof:

- Repository search outside `drivers/clk/ti/` and outside `include/linux/clk/ti.h` found no users of `linux/clk/ti.h`, `omap2_clk_*`, `ti_dt_clk_*`, `ti_clk_setup*`, `ti_clk_get_features`, `omap3430_dt_clk_init`, `am33xx_dt_clk_init`, `dra7xx_dt_clk_init`, or `clkhwops_omap2xxx_dpll`.
- Remaining generic references to `ARCH_OMAP2PLUS` in other driver Kconfig files and headers are not users of this clock-provider directory and are left for separate proof passes.
- `include/dt-bindings/clock/*` entries and non-clock TI platform headers are intentionally left for separate include/platform proof passes.

Planned patch class: `delete_plus_build_wiring`

Planned source edits:

- Delete `drivers/clk/ti/`.
- Delete dead declaration/data header `include/linux/clk/ti.h`.
- Remove `source "drivers/clk/ti/Kconfig"` from `drivers/clk/Kconfig`.
- Remove `obj-y += ti/` from `drivers/clk/Makefile`.

Required verification:

- Verify `drivers/clk/ti/` and `include/linux/clk/ti.h` are gone from the worktree.
- Run `git diff --check && git diff --cached --check`.
- Run `JOBS=32 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh`.
- After commit, run `git show --stat --oneline --decorate HEAD`, `git show --check --oneline HEAD`, targeted `git show -- drivers/clk/Kconfig drivers/clk/Makefile include/linux/clk/ti.h`, and `git status --short --untracked-files=no`.
