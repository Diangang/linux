# Stage 5 tools directory deletion pass review

Date: 2026-05-02

Patch class: delete_plus_build_wiring

Commit: `616246a3ce74` (`tools: remove non-validation user tools`)

Scope:

- Removed user-space tools, tests, performance tooling, tracing helpers,
  verification helpers, power/thermal utilities, USB/virtio examples, and
  non-x86 tools architecture headers outside the fixed storage contract.
- Kept the kernel-build tool surface required by current validation:
  `tools/objtool/`, `tools/build/`, `tools/scripts/`, `tools/include/`,
  `tools/arch/x86/`, `tools/lib/subcmd/`, `tools/lib/bpf/`, and
  `tools/bpf/resolve_btfids/`.
- Removed `lib/test_bitmap.c` because it directly included the deleted
  `tools/testing/selftests/kselftest_module.h` helper.

Mechanical fallout:

- Reduced `tools/Makefile` to retained kernel-build tool targets.
- Removed `bpftool` wiring from `tools/bpf/Makefile`.
- Removed top-level kselftest targets after deleting `tools/testing/selftests/`.
- Removed perf source package rules after deleting `tools/perf/`.
- Removed kernel-doc build hooks after deleting `tools/docs/kernel-doc`.
- Removed `CONFIG_TEST_BITMAP` and its lib build entry after deleting
  `lib/test_bitmap.c`.

Validation:

- `git diff --check -- Makefile scripts tools lib include/drm`: passed.
- `JOBS=32 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh`:
  passed.
- `git show --check --oneline HEAD`: passed after commit.
- x86_64 QEMU Minix storage log:
  `.codex-qemu-kernels/logs/qemu/minix-storage-x86_64-20260502-212248-attempt1.log`
- arm64 QEMU Minix storage log:
  `.codex-qemu-kernels/logs/qemu/minix-storage-arm64-20260502-212251-attempt1.log`
- Metrics: `.codex-qemu-kernels/metrics/metrics-20260502-212252.txt`

Findings:

- No validation regression found for the fixed NVMe plus SCSI HDD Minix
  contract.
- The tools deletion pass is committed as `616246a3ce74`.
- Remaining `tools/` content is the build-tool support surface retained for
  objtool, possible BTF ID resolution, and shared host headers/libraries.
- Non-config source fallout is recorded in the compatibility ledger for later
  rollback review.
