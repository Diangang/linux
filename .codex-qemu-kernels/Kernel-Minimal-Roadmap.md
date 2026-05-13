# Linux minimal core trimming roadmap

This workspace trims the Linux tree to the smallest kernel that still satisfies
the fixed x86_64 plus arm64 QEMU Minix storage contract.

The source tree and the control workspace have separate responsibilities:

- Kernel source commits remove source, Kconfig, Makefile wiring, or mechanical
  dead references.
- `.codex-qemu-kernels/` stores recovery state, validation scripts, fixed
  config inputs, and current project notes.
- Do not mix source commits with control-workspace maintenance commits.

## Fixed validation contract

Every accepted source change must pass:

```sh
JOBS=32 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh
```

The contract is:

- build x86_64 kernel image;
- build arm64 kernel image;
- build generated initramfs for both architectures;
- boot both architectures under QEMU;
- attach NVMe as `/dev/nvme0n1`;
- attach HDD-like SCSI disk as `/dev/sda`;
- format both disks as Minix with `mkfs.minix -3`;
- mount, write, read, sync, and unmount both filesystems;
- observe `CODEX_MINIX_TEST_PASS`.

Other architectures, filesystems, storage drivers, debug features, tests, and
platform support are out of scope unless a direct dependency on this contract is
proven.

## Modification policy

Prefer the largest safe deletion unit:

1. whole directory;
2. whole file;
3. Kconfig/Makefile/build wiring;
4. file-internal guarded dead code.

Allowed patch classes:

- pure deletion;
- deletion plus mechanical build/config fallout;
- permanent no-op collapse for optional facilities already disabled on both
  target architectures when broad include dependencies still require a header.

Stop for human review if a change would alter runtime semantics in core boot,
VFS, block, NVMe, SCSI, Minix, locking, lifetime, syscall/UAPI, or I/O paths.

## Current progress

Current source HEAD:

```text
9423d475c3c4 refactor(\u88c1\u526a): \u5220\u9664\u53cc\u5e73\u53f0\u5173\u95ed\u914d\u7f6e\u6b8b\u7559
```

The latest completed pass removed stale config and guarded-code residue for the
symbols that were closed on both x86_64 and arm64:

```text
APM
ARCH_BCM
ARCH_MICROCHIP
ARCH_NXP
ARMV8_DEPRECATED
KASAN
KCSAN
KFENCE
KGDB
PROCESSOR_SELECT
TEST_RUNTIME
```

Validation passed on 2026-05-13 with:

```sh
JOBS=32 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh
```

Both x86_64 and arm64 QEMU runs reached `CODEX_MINIX_TEST_PASS`.

## Next direction

Continue Stage 6 as a residual disabled-config/source-surface reduction loop:

1. Regenerate the x86_64 and arm64 disabled-config intersection from current
   code.
2. Classify remaining symbols into safe deletion, keep disabled, and unclear
   dependency groups.
3. Select one tightly related safe group.
4. Remove its Kconfig, Makefile, and guarded code residue.
5. Run residue scans, `git diff --check`, and the full JOBS=32 validation
   contract.
6. Commit exactly one source change for the selected group.

Historical reports and metrics were compacted away on 2026-05-13. The current
analysis/progress/plan source is `.codex-qemu-kernels/CURRENT_PROJECT_STATE.md`.
