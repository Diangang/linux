# Stage 6 review: Sunxi clock-provider deletion

## Commit

- `3a93d1b043e9 refactor(裁剪clk): 删除Sunxi时钟驱动`

## Scope

- Patch class: `delete_plus_build_wiring`.
- Deleted the whole `drivers/clk/sunxi-ng/` source directory.
- Deleted `include/linux/clk/sunxi-ng.h`, whose helper declarations had no users outside the removed leaf.
- Removed only the direct parent wiring:
  - `drivers/clk/Kconfig`: `source "drivers/clk/sunxi-ng/Kconfig"`
  - `drivers/clk/Makefile`: `obj-y += sunxi-ng/`
- The physical source directory `drivers/clk/sunxi-ng` is gone.

## Validation

- Command: `JOBS=32 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh`
- Result: passed.
- x86_64 QEMU log: `.codex-qemu-kernels/logs/qemu/minix-storage-x86_64-20260505-193636-attempt1.log`
- arm64 QEMU log: `.codex-qemu-kernels/logs/qemu/minix-storage-arm64-20260505-193639-attempt1.log`
- Metrics: `.codex-qemu-kernels/metrics/metrics-20260505-193640.txt`
- The generated initramfs QEMU tests passed on attempt 1 for both x86_64 and arm64; both guests mounted, wrote, read, synced, and unmounted Minix on `/dev/nvme0n1` and `/dev/sda`.

## Review

- `git show --stat --oneline --decorate HEAD` shows 85 files changed and 27881 deletions, all in `drivers/clk/sunxi-ng/`, `include/linux/clk/sunxi-ng.h`, and the two parent wiring lines.
- `git show --check --oneline HEAD` reported no whitespace errors.
- `git show -- drivers/clk/Kconfig drivers/clk/Makefile include/linux/clk/sunxi-ng.h` confirms only the direct Kconfig source line, direct Makefile descent line, and dead helper header were removed.
- Finding: clean.
