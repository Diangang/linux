# Stage 6 dependency ledger: Sophgo clock provider deletion

Candidate: `drivers/clk/sophgo/`

Selection assumptions:

- The fixed validation contract is limited to x86_64 and arm64 QEMU boot plus NVMe `/dev/nvme0n1` and SCSI `/dev/sda` Minix mount/write/read/sync/umount.
- Sophgo CV18xx/SG204x platform clock support is outside the fixed QEMU storage contract.
- Directory-level deletion remains available in `drivers/`, so whole-file and file-internal trimming stay deferred.

Coarse size check:

- `drivers/clk/sophgo/`: 16 files, 8040 lines, largest remaining `drivers/clk` leaf after the Broadcom deletion.

Build/config wiring:

- Parent config edge: `drivers/clk/Kconfig` sources `drivers/clk/sophgo/Kconfig`.
- Parent build edge: `drivers/clk/Makefile` adds `obj-y += sophgo/`.
- `drivers/clk/sophgo/Kconfig` symbols depend on `ARCH_SOPHGO || COMPILE_TEST` or on other Sophgo clock symbols.

Fixed-config proof:

- Fixed arm64 `.config` and `arch/arm64/configs/defconfig` have `# CONFIG_ARCH_SOPHGO is not set`.
- Fixed x86_64 `.config`, fixed arm64 `.config`, and arm64 defconfig have no enablement for `CLK_SOPHGO*`.

Generated dependency proof:

- `.cmd` scans under `.codex-qemu-kernels/build-x86_64` and `.codex-qemu-kernels/build-arm64` found no dependency edge for `drivers/clk/sophgo`, `clk/sophgo`, `clk-sophgo`, `clk-cv18*`, `clk-sg204*`, `CLK_SOPHGO`, `ARCH_SOPHGO`, `SG204*`, or `CV1800/CV18*`.
- Pre-delete fixed build directories contain only empty aggregate artifacts:
  - `.codex-qemu-kernels/build-x86_64/drivers/clk/sophgo/{built-in.a,modules.order,.built-in.a.cmd,.modules.order.cmd}`
  - `.codex-qemu-kernels/build-arm64/drivers/clk/sophgo/{built-in.a,modules.order,.built-in.a.cmd,.modules.order.cmd}`

External reference proof:

- Repository search outside `drivers/clk/sophgo/` found no code users of Sophgo clock driver symbols.
- Remaining Sophgo references are arm64 platform Kconfig, dt-binding headers, and independent DMA/PCI/IRQ platform residue; these are intentionally left for separate proof passes.

Planned patch class: `delete_plus_build_wiring`

Planned source edits:

- Delete `drivers/clk/sophgo/`.
- Remove `source "drivers/clk/sophgo/Kconfig"` from `drivers/clk/Kconfig`.
- Remove `obj-y += sophgo/` from `drivers/clk/Makefile`.

Required verification:

- Verify `drivers/clk/sophgo/` is gone from the worktree.
- Run `git diff --check && git diff --cached --check`.
- Run `JOBS=32 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh`.
- After commit, run `git show --stat --oneline --decorate HEAD`, `git show --check --oneline HEAD`, targeted `git show -- drivers/clk/Kconfig drivers/clk/Makefile drivers/clk/sophgo/Kconfig`, and `git status --short --untracked-files=no`.
