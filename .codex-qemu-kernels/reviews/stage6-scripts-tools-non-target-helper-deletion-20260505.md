# Stage 6 scripts/tools non-target helper deletion review

Date: 2026-05-05
Commit: 74211db942e2

## Scope

- Deleted 211 files from `scripts/` and `tools/`.
- Patch is pure deletion: 0 additions, 22862 deletions.
- No Makefile, Kconfig, runtime C logic, or build wiring was changed.

## Removed file families

- Kconfig interactive frontends and tests:
  `mconf`, `nconf`, `gconf`, `qconf`, icons, `lxdialog`, and
  `scripts/kconfig/tests/`.
- Kconfig non-contract helpers:
  `merge_config.sh` and `streamline_config.pl`.
- DTC ancillary userspace tools:
  `fdtget`, `fdtput`, `dt_to_config`, `dtx_diff`,
  `dt-extract-compatibles`, and `of_unittest_expect`.
- Top-level diagnostic, maintenance, documentation, and convenience scripts
  outside the fixed kernel image build/run target.
- `tools/build` feature probing framework and probe programs, which are not
  used by the retained objtool/libbpf/resolve_btfids path for the fixed build.

## Kept build chain

- `scripts/basic/fixdep`
- `scripts/kconfig/conf` source set
- `scripts/dtc/dtc` and `fdtoverlay` source set
- `scripts/kallsyms`, `sorttable`, `mod`, syscall generation scripts, and
  other Kbuild-required helpers
- `tools/objtool`, `tools/lib/subcmd`, `tools/lib/bpf`,
  `tools/bpf/resolve_btfids`, and required `tools/include` headers

## Validation

Command:

```sh
JOBS=32 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh
```

Result: passed.

- x86_64 build produced
  `.codex-qemu-kernels/build-x86_64/arch/x86/boot/bzImage`.
- arm64 build produced
  `.codex-qemu-kernels/build-arm64/arch/arm64/boot/Image`.
- x86_64 QEMU Minix storage test passed on attempt 1:
  `.codex-qemu-kernels/logs/qemu/minix-storage-x86_64-20260505-182657-attempt1.log`.
- arm64 QEMU Minix storage test passed on attempt 1:
  `.codex-qemu-kernels/logs/qemu/minix-storage-arm64-20260505-182700-attempt1.log`.
- Metrics:
  `.codex-qemu-kernels/metrics/metrics-20260505-182701.txt`.

## Review Result

No findings for the fixed x86_64/arm64 kernel image build and QEMU Minix
storage runtime contract. Residual risk is limited to out-of-contract developer
targets such as interactive Kconfig frontends, localmodconfig, Kconfig unit
tests, DTC standalone utilities, and tools feature detection targets.
