# Stage 2 EFS removal ledger

## Target

Remove the SGI EFS read-only filesystem implementation as one complete
filesystem feature.

## Dependency check

EFS is not part of the fixed validation contract. The required filesystem is
Minix, mounted from generated initramfs tests over QEMU NVMe and SCSI HDD
devices.

Local references before deletion:

- `fs/Kconfig` sourced `fs/efs/Kconfig`.
- `fs/Makefile` built `fs/efs/` through `CONFIG_EFS_FS`.
- `MAINTAINERS` had an orphan EFS entry for `fs/efs/`.
- stale `CONFIG_EFS_FS=m` selections existed in mips and powerpc defconfigs.

After deletion, exact `CONFIG_EFS_FS` and `fs/efs` build wiring references were
absent from `MAINTAINERS`, `fs`, and `arch`. Broader `EFS_FS` text matches are
from unrelated filesystems such as BEFS, ORANGEFS, and ZONEFS.

## Patch class

`delete_plus_build_wiring`

No runtime logic is changed and no stubs are added.

## Pre-validation review

- `git diff --stat` shows only the EFS implementation deletion and related
  build/config cleanup.
- `git diff --check` passed.
