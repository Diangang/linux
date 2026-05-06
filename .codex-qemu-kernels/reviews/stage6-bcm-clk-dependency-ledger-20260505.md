# Stage 6 dependency ledger: Broadcom clock provider deletion

Candidate: `drivers/clk/bcm/`

Selection assumptions:

- The fixed validation contract is limited to x86_64 and arm64 QEMU boot plus NVMe `/dev/nvme0n1` and SCSI `/dev/sda` Minix mount/write/read/sync/umount.
- Broadcom BCM/Raspberry Pi/iProc platform clock support is outside the fixed QEMU storage contract.
- Directory-level deletion remains available in `drivers/`, so whole-file and file-internal trimming stay deferred.

Coarse size check:

- `drivers/clk/bcm/`: 24 files, 10236 lines, largest remaining `drivers/clk` leaf after the TI deletion.

Build/config wiring:

- Parent config edge: `drivers/clk/Kconfig` sources `drivers/clk/bcm/Kconfig`.
- Parent build edge: `drivers/clk/Makefile` adds `obj-y += bcm/`.
- `drivers/clk/bcm/Kconfig` symbols depend on Broadcom platform families or compile-test gates: `ARCH_BCM2835`, `ARCH_BRCMSTB`, `ARCH_BCMBCA`, `BMIPS_GENERIC`, `ARCH_BCM_MOBILE`, `ARCH_BCM_CYGNUS`, `ARCH_BCM_HR2`, `ARCH_BCM_5301X`, `ARCH_BCM_NSP`, `ARCH_BCM_IPROC`, `RASPBERRYPI_FIRMWARE`, and `COMPILE_TEST`.
- `arch/arm64/Kconfig.platforms` selects `COMMON_CLK_IPROC` only under `ARCH_BCM`, which is disabled in the fixed arm64 config.

Fixed-config proof:

- Fixed arm64 `.config` and `arch/arm64/configs/defconfig` have `# CONFIG_ARCH_BCM is not set`.
- Fixed x86_64 `.config`, fixed arm64 `.config`, and arm64 defconfig have no enablement for `CLK_BCM*`, `COMMON_CLK_IPROC`, `CLK_RASPBERRYPI`, `RASPBERRYPI_FIRMWARE`, or the Broadcom platform symbols.

Generated dependency proof:

- `.cmd` scans under `.codex-qemu-kernels/build-x86_64` and `.codex-qemu-kernels/build-arm64` found no dependency edge for `drivers/clk/bcm`, `clk/bcm`, `clk-bcm`, `clk-kona`, `clk-iproc`, `clk-raspberrypi`, `COMMON_CLK_IPROC`, `CLK_BCM*`, `CLK_RASPBERRYPI`, `ARCH_BCM*`, `ARCH_BRCM*`, `RASPBERRYPI_FIRMWARE`, or `BMIPS_GENERIC`.
- Pre-delete fixed build directories contain only empty aggregate artifacts:
  - `.codex-qemu-kernels/build-x86_64/drivers/clk/bcm/{built-in.a,modules.order,.built-in.a.cmd,.modules.order.cmd}`
  - `.codex-qemu-kernels/build-arm64/drivers/clk/bcm/{built-in.a,modules.order,.built-in.a.cmd,.modules.order.cmd}`

External reference proof:

- There is no `include/linux/clk/bcm*.h` or matching Broadcom clock header under `include/`.
- Repository search outside `drivers/clk/bcm/` found no code users for the Broadcom clock symbols or source names. The remaining `drivers/firmware/raspberrypi.c` string registering `"raspberrypi-clk"` is not active in fixed configs and is left as unrelated firmware/platform residue for a later proof pass.
- Broadcom dt-bindings and non-clock platform references are intentionally left for separate include/platform proof passes.

Planned patch class: `delete_plus_build_wiring`

Planned source edits:

- Delete `drivers/clk/bcm/`.
- Remove `source "drivers/clk/bcm/Kconfig"` from `drivers/clk/Kconfig`.
- Remove `obj-y += bcm/` from `drivers/clk/Makefile`.

Required verification:

- Verify `drivers/clk/bcm/` is gone from the worktree.
- Run `git diff --check && git diff --cached --check`.
- Run `JOBS=32 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh`.
- After commit, run `git show --stat --oneline --decorate HEAD`, `git show --check --oneline HEAD`, targeted `git show -- drivers/clk/Kconfig drivers/clk/Makefile drivers/clk/bcm/Kconfig`, and `git status --short --untracked-files=no`.
