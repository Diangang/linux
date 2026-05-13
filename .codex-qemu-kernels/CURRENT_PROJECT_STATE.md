# Current project state

Last refreshed: 2026-05-13

## Goal

Trim this Linux kernel tree to the smallest x86_64 plus arm64 kernel that still
passes the fixed QEMU Minix storage contract. The required runtime path is:

- generated initramfs through `rdinit=/init`;
- NVMe disk at `/dev/nvme0n1`;
- HDD-like SCSI disk at `/dev/sda`;
- Minix filesystem on both disks;
- mount, write, read, sync, and unmount on both disks;
- success marker `CODEX_MINIX_TEST_PASS`.

## Current code base

Current source HEAD:

```text
9423d475c3c4 refactor(\u88c1\u526a): \u5220\u9664\u53cc\u5e73\u53f0\u5173\u95ed\u914d\u7f6e\u6b8b\u7559
```

Latest validation:

```sh
JOBS=32 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh
```

Result: passed on 2026-05-13. Both target architectures reached
`CODEX_MINIX_TEST_PASS`.

## Completed cleanup status

The tree has already removed large non-target areas and many residual feature
surfaces. The latest pass specifically removed Kconfig, Makefile, and guarded
code residue for this disabled-on-both-targets group:

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

Public headers for optional sanitizer/debug facilities were collapsed to
permanent no-op definitions where broad include dependencies still require the
header to exist.

## Control workspace cleanup

The control workspace was compacted on 2026-05-13:

- removed historical `reviews/` ledgers;
- removed historical `metrics/` snapshots;
- removed temporary candidate lists and old config-analysis files;
- removed generated build trees, raw disks, initramfs images, rootfs staging
  directories, and logs;
- kept validation scripts, fixed configs, prompts, roadmap, test contract, and
  this current-state document.

The removed artifacts are either historical summaries now superseded by this
file or generated outputs that can be recreated by the validation scripts.

## Keep list

Keep only what the fixed contract requires or what is a direct dependency of
that path:

- x86_64 and arm64 boot paths;
- initramfs support and devtmpfs;
- VFS/core mount and file I/O;
- Minix;
- block core;
- NVMe host path needed by QEMU;
- SCSI core, SCSI disk, and virtio-scsi path needed by QEMU;
- PCI, interrupt, firmware, timer, memory, and driver-core dependencies needed
  by those devices.

Everything else remains a candidate until proven required.

## Next plan

1. Regenerate the disabled-config intersection for x86_64 and arm64 from the
   current tree.
2. Classify the remaining disabled symbols into safe-delete, keep-disabled, and
   unclear-dependency groups.
3. Pick one coherent safe-delete group for the next source commit.
4. Remove its Kconfig entries, build wiring, and guarded code.
5. Scan for target-symbol residue.
6. Run `git diff --check`.
7. Run `JOBS=32 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh`.
8. Commit only the source changes after validation passes.

Use `JOBS=1` only for short diagnostic reruns when a parallel build failure
hides the first real compiler error.
