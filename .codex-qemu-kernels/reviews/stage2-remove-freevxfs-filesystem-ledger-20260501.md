# Stage 2 dependency ledger: remove freevxfs filesystem

Target: FreeVxFS / Veritas VxFS filesystem support (`CONFIG_VXFS_FS`,
`fs/freevxfs/`).

Assumptions:
- The fixed validation contract requires only Minix as the mounted test
  filesystem.
- x86_64 and arm64 are the only validation architectures.
- NVMe, SCSI disk, virtio-scsi, initramfs, devtmpfs, proc/sysfs, and VFS core
  paths must remain untouched.
- Other architecture defconfig fallout is cleanup for stale Kconfig references,
  not a validation blocker.

Dependency evidence:
- `fs/freevxfs/Kconfig` defines a standalone read-only Veritas VxFS filesystem
  option, `CONFIG_VXFS_FS`, depending only on `BLOCK` and selecting
  `BUFFER_HEAD`.
- `fs/freevxfs/Makefile` builds only the `freevxfs` module objects under
  `CONFIG_VXFS_FS`.
- `fs/Kconfig` includes only `source "fs/freevxfs/Kconfig"` for this feature.
- `fs/Makefile` includes only `obj-$(CONFIG_VXFS_FS) += freevxfs/` for this
  implementation.
- `MAINTAINERS` has a dedicated `FREEVXFS FILESYSTEM` block whose only file
  pattern is `fs/freevxfs/`.
- `CREDITS` has one direct `freevxfs driver` description line for the removed
  driver.
- Stale `CONFIG_VXFS_FS=m` defconfig selections exist under MIPS and PowerPC;
  those architectures are outside validation scope, but leaving dead Kconfig
  selections would keep removed-feature references in source configs.
- No x86_64 or arm64 defconfig reference to `CONFIG_VXFS_FS` was found.

Decision:
- Remove `fs/freevxfs/`.
- Remove the `fs/Kconfig` source line and `fs/Makefile` object line.
- Remove the `FREEVXFS FILESYSTEM` MAINTAINERS block.
- Remove the direct `freevxfs driver` CREDITS line.
- Remove stale `CONFIG_VXFS_FS=m` lines from non-scope defconfigs.
- Do not edit unrelated VxFS mentions in comments or test filesystem name lists.

Patch class: `delete_plus_build_wiring`.

Verification before commit:
- `JOBS=8 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh`
- `git show --stat --oneline --decorate HEAD`
- `git show --check --oneline HEAD`
- targeted `git show -- <changed source files>`
