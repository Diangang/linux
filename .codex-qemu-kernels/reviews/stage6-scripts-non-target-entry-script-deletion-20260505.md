# Stage 6 scripts non-target entry script deletion review

Date: 2026-05-05
Commit: ac031758aaa4

## Scope

- Deleted 8 files from `scripts/`.
- Patch is pure deletion: 0 additions, 676 deletions.
- No Makefile, Kconfig, runtime C logic, or build wiring was changed.

## Removed Scripts

- `scripts/check-sysctl-docs`
- `scripts/check_extable.sh`
- `scripts/depmod.sh`
- `scripts/headers_install.sh`
- `scripts/mkuboot.sh`
- `scripts/nsdeps`
- `scripts/objdiff`
- `scripts/xen-hypercalls.sh`

These files serve out-of-contract targets or diagnostic/install helper paths:
sysctl documentation checks, extable debugging advice, `modules_install`,
`headers_install`, U-Boot image wrapping, namespace dependency generation,
object diffing, and Xen hypercall header generation.

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
  `.codex-qemu-kernels/logs/qemu/minix-storage-x86_64-20260505-183108-attempt1.log`.
- arm64 QEMU Minix storage test passed on attempt 1:
  `.codex-qemu-kernels/logs/qemu/minix-storage-arm64-20260505-183112-attempt1.log`.
- Metrics:
  `.codex-qemu-kernels/metrics/metrics-20260505-183113.txt`.

## Review Result

No findings for the fixed x86_64/arm64 kernel image build and QEMU Minix
storage runtime contract. Remaining shell-like helpers are either directly
observed in the fixed build, referenced by current top-level build preparation,
or tied to config evaluation paths that should be removed only with explicit
build/config cleanup rather than file deletion alone.
