# Stage 2 dependency ledger: remove befs filesystem

Target: BeOS filesystem support (`CONFIG_BEFS_FS`, `fs/befs/`).

Assumptions:
- The fixed validation contract requires only Minix as the mounted test
  filesystem.
- x86_64 and arm64 are the only validation architectures.
- NVMe, SCSI disk, virtio-scsi, initramfs, devtmpfs, proc/sysfs, and VFS core
  paths must remain untouched.
- Other architecture defconfig fallout is cleanup for stale Kconfig references,
  not a validation blocker.

Dependency evidence:
- `fs/befs/Kconfig` defines standalone read-only BeOS filesystem support,
  `CONFIG_BEFS_FS`, depending on `BLOCK` and selecting `BUFFER_HEAD` and `NLS`.
- `CONFIG_BEFS_DEBUG` is a child option depending on `CONFIG_BEFS_FS`; deleting
  the Kconfig file removes both together.
- `fs/befs/Makefile` builds only the `befs` module objects under
  `CONFIG_BEFS_FS`.
- `fs/Kconfig` includes only `source "fs/befs/Kconfig"` for this feature.
- `fs/Makefile` includes only `obj-$(CONFIG_BEFS_FS) += befs/` for this
  implementation.
- `Documentation/filesystems/index.rst` and
  `Documentation/filesystems/befs.rst` document only the removed filesystem.
- `MAINTAINERS` has a dedicated `BEFS FILE SYSTEM` block whose file patterns
  are the BeFS documentation and `fs/befs/`.
- Stale `CONFIG_BEFS_FS=m` defconfig selections exist under MIPS and PowerPC;
  those architectures are outside validation scope, but the selections would
  reference a removed Kconfig symbol.
- No x86_64 or arm64 defconfig reference to `CONFIG_BEFS_FS` was found.
- The statmount selftest known-filesystem list has a direct `befs` token; it is
  removed as dead test metadata for the deleted filesystem.

Decision:
- Remove `fs/befs/`.
- Remove the `fs/Kconfig` source line and `fs/Makefile` object line.
- Remove BeFS documentation and its documentation index entry.
- Remove the `BEFS FILE SYSTEM` MAINTAINERS block.
- Remove stale `CONFIG_BEFS_FS=m` lines from non-scope defconfigs.
- Remove the direct `befs` token from the statmount known-filesystem list.

Patch class: `delete_plus_build_wiring`.

Verification before commit:
- `JOBS=8 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh`
- `git show --stat --oneline --decorate HEAD`
- `git show --check --oneline HEAD`
- targeted `git show -- <changed source files>`
