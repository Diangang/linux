# Stage 5 lib/kernel/mm directory pass

Date: 2026-05-02

Scope:

- Removed debug, test, sanitizer, livepatch/liveupdate, DAMON, and KUnit
  implementation directories from `kernel/`, `lib/`, and `mm/`.
- Removed stale build and Kconfig wiring that still entered or sourced those
  deleted directories.

Deleted directory groups:

- `kernel/debug/`, `kernel/gcov/`, `kernel/kcsan/`, `kernel/livepatch/`,
  `kernel/liveupdate/`
- `lib/kunit/`, `lib/tests/`, `lib/math/tests/`, `lib/crc/tests/`,
  `lib/crypto/tests/`, `lib/raid6/test/`, `lib/raid/xor/tests/`,
  `lib/test_fortify/`
- `mm/damon/`, `mm/kasan/`, `mm/kfence/`, `mm/kmsan/`, `mm/tests/`

Compatibility ledger:

- `arch/arm64/kernel/entry-common.c`: added an explicit
  `asm/debug-monitors.h` include after the clean arm64 build exposed the
  existing `try_step_suspended_breakpoints()` declaration dependency. Revisit
  this include if arm64 hw-breakpoint/debug-monitor paths are later deleted.

Validation:

- Command: `JOBS=32 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh`
- x86_64 QEMU log:
  `.codex-qemu-kernels/logs/qemu/minix-storage-x86_64-20260502-213916-attempt1.log`
- arm64 QEMU log:
  `.codex-qemu-kernels/logs/qemu/minix-storage-arm64-20260502-213919-attempt1.log`
- Metrics: `.codex-qemu-kernels/metrics/metrics-20260502-213920.txt`

Result:

- x86_64 build passed.
- arm64 build passed.
- x86_64 Minix NVMe and SCSI storage boot test passed.
- arm64 Minix NVMe and SCSI storage boot test passed.
