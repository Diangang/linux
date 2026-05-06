# Stage 2 AFFS removal bugfix review

Patch: `stage2-remove-affs-filesystem`
Commit: `89d851e275ab fs: remove AFFS filesystem`
Patch class: `delete_plus_build_wiring`
Status: clean

## Mechanical Bugfix

The bugfix removed stale `CONFIG_AFFS_FS=m` selections from m68k, mips, and powerpc defconfigs after the AFFS Kconfig entry was deleted.

No runtime logic, function bodies, control flow, data structure semantics, error handling, locking, reference counting, I/O behavior, mount/read/write behavior, syscall/UAPI behavior, device names, or stubs were changed.

## Scope Checks

- `git diff --check`: passed.
- `rg -n "CONFIG_AFFS_FS|source \"fs/affs/Kconfig\"|obj-\\$\\(CONFIG_AFFS_FS\\)|fs/affs" arch block fs --glob '!fs/affs/**'`: no remaining arch/block/fs build or Kconfig references outside the deleted AFFS tree.
- `git show --check --oneline HEAD`: passed.

Documentation, MAINTAINERS, CREDITS, and shared UAPI AFFS hardblock references were left unchanged because they are outside this mechanical deletion-fallout bugfix scope and do not create build wiring to the deleted filesystem.

## Validation

Command:

```sh
JOBS=8 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh
```

Result: passed.

Evidence:

- `.codex-qemu-kernels/logs/qemu/minix-storage-x86_64-20260430-233450-attempt1.log`
- `.codex-qemu-kernels/logs/qemu/minix-storage-arm64-20260430-233454-attempt1.log`
- `.codex-qemu-kernels/metrics/metrics-20260430-233456.txt`

Validation remained initramfs-based. No Debian qcow2 root filesystem or virtio-blk root-device dependency was introduced.

## Review

The committed source diff removes one complete filesystem feature family, AFFS, plus mechanical stale build/config fallout. The change remains outside the fixed Minix plus NVMe/SCSI-HDD validation contract and preserves the shared AFFS hardblock UAPI used by Amiga partition parsing.

Review finding: none.
