# Bugfix codex process recovery

Scope: control-state recovery after `codex_process_failed`.

Evidence:
- Latest normal run ended with remote compaction failure after commit
  `6dba022ee751 refactor(裁剪firmware): 删除PSCI checker自检`.
- The failure was not a kernel build, QEMU boot, or storage validation failure.
- Existing PSCI checker review already recorded full fixed storage validation:
  x86_64 and arm64 QEMU initramfs Minix tests reached `CODEX_MINIX_TEST_PASS`.

Mechanical fix:
- Updated `.codex-qemu-kernels/state.json` from bugfix stop state back to
  `next_patch`.
- Recorded commit `6dba022ee751` as the current validated Stage 6 commit.
- Marked the PSCI checker review state clean and updated the
  `drivers/firmware/` continuation note.

Source changes:
- None.

Validation plan:
- `jq empty .codex-qemu-kernels/state.json`
- `git status --porcelain=v1 -uall -- ':!/.codex-qemu-kernels'`
- `git show --check --oneline HEAD`
- `find drivers/firmware -type d -empty -print | sort`
- `JOBS=32 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh`

Validation result:
- `jq empty .codex-qemu-kernels/state.json`: passed.
- Source status excluding `.codex-qemu-kernels`: clean.
- `git show --check --oneline HEAD`: clean.
- Empty firmware directory check: clean.
- Full fixed storage validation passed:
  - x86_64 log:
    `.codex-qemu-kernels/logs/qemu/minix-storage-x86_64-20260505-212804-attempt1.log`
  - arm64 log:
    `.codex-qemu-kernels/logs/qemu/minix-storage-arm64-20260505-212807-attempt1.log`
  - metrics:
    `.codex-qemu-kernels/metrics/metrics-20260505-212808.txt`
- Both QEMU guests used initramfs `rdinit=/init` and reached
  `CODEX_MINIX_TEST_PASS`.

Review result:
- Clean.
- No source commit or amend was needed because the bugfix changed only
  `.codex-qemu-kernels/` control state and review records.
- Normal continuation is ready at `drivers/firmware/` next-patch selection.
