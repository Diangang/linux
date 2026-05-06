# Stage 6 dependency ledger: Spreadtrum clock provider deletion

Candidate: `drivers/clk/sprd/`

Selection assumptions:

- The fixed validation contract is limited to x86_64 and arm64 QEMU boot plus NVMe `/dev/nvme0n1` and SCSI `/dev/sda` Minix mount/write/read/sync/umount.
- Spreadtrum/Sprd SC986x/UMS512 platform clock support is outside the fixed QEMU storage contract.
- Directory-level deletion remains available in `drivers/`, so whole-file and file-internal trimming stay deferred.

Coarse size check:

- `drivers/clk/sprd/`: 17 files, 7426 lines, largest remaining `drivers/clk` leaf after the Sophgo deletion.

Build/config wiring:

- Parent config edge: `drivers/clk/Kconfig` sources `drivers/clk/sprd/Kconfig`.
- Parent build edge: `drivers/clk/Makefile` adds `obj-y += sprd/`.
- `drivers/clk/sprd/Kconfig` symbols depend on `ARCH_SPRD || COMPILE_TEST` or on other Sprd clock symbols.

Fixed-config proof:

- Fixed arm64 `.config` and `arch/arm64/configs/defconfig` have `# CONFIG_ARCH_SPRD is not set` and `# CONFIG_SERIAL_SPRD is not set`.
- Fixed x86_64 `.config`, fixed arm64 `.config`, and arm64 defconfig have no enablement for `SPRD_COMMON_CLK` or `SPRD_*_CLK`.

Generated dependency proof:

- `.cmd` scans under `.codex-qemu-kernels/build-x86_64` and `.codex-qemu-kernels/build-arm64` found no dependency edge for `drivers/clk/sprd`, `clk/sprd`, `clk-sprd`, `sc986*`, `ums512`, `SPRD_COMMON_CLK`, `SPRD_*_CLK`, or `ARCH_SPRD`.
- Pre-delete fixed build directories contain only empty aggregate artifacts:
  - `.codex-qemu-kernels/build-x86_64/drivers/clk/sprd/{built-in.a,modules.order,.built-in.a.cmd,.modules.order.cmd}`
  - `.codex-qemu-kernels/build-arm64/drivers/clk/sprd/{built-in.a,modules.order,.built-in.a.cmd,.modules.order.cmd}`

External reference proof:

- There is no `include/linux/clk/sprd*.h` clock header.
- Repository search outside `drivers/clk/sprd/` found no code users of Sprd clock driver symbols.
- Remaining Sprd references are arm64 platform Kconfig, dt-binding headers, and independent clocksource/DMA/IOMMU/serial platform residue; these are intentionally left for separate proof passes.

Planned patch class: `delete_plus_build_wiring`

Planned source edits:

- Delete `drivers/clk/sprd/`.
- Remove `source "drivers/clk/sprd/Kconfig"` from `drivers/clk/Kconfig`.
- Remove `obj-y += sprd/` from `drivers/clk/Makefile`.

Required verification:

- Verify `drivers/clk/sprd/` is gone from the worktree.
- Run `git diff --check && git diff --cached --check`.
- Run `JOBS=32 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh`.
- After commit, run `git show --stat --oneline --decorate HEAD`, `git show --check --oneline HEAD`, targeted `git show -- drivers/clk/Kconfig drivers/clk/Makefile drivers/clk/sprd/Kconfig`, and `git status --short --untracked-files=no`.
