# Stage 6 scripts directory dependency ledger

Target: `scripts/`

Assumptions:
- Fixed validation scope is x86_64 plus arm64 QEMU boot with NVMe `/dev/nvme0n1`,
  SCSI HDD `/dev/sda`, and Minix mount/write/read/sync/umount.
- This pass may delete only whole directories or whole files that are not used by
  the fixed validation builds.
- Build-host helpers referenced by top-level Makefile/Kconfig, generated `.cmd`
  dependency files, or source include paths are kept.

Evidence:
- Current source status excluding `.codex-qemu-kernels` is clean.
- `.codex-qemu-kernels/build-x86_64` and
  `.codex-qemu-kernels/build-arm64` contain 3392 generated `.cmd` files for
  dependency evidence.
- `Kbuild` prepares `include/linux/atomic/{atomic-arch-fallback.h,
  atomic-instrumented.h,atomic-long.h}` by checking their embedded SHA only; it
  does not invoke files under `scripts/atomic/`.
- The only non-control-tree references to `scripts/atomic` are comments in the
  already generated `include/linux/atomic/*` headers.
- No fixed-build `.cmd` dependency references `scripts/atomic/` or
  `scripts/dummy-tools/`.
- `scripts/dummy-tools/` is referenced only by strings/comments inside its own
  files.

Classification:
- Delete: `scripts/atomic/`. The generated atomic headers are already in
  `include/linux/atomic/`; fixed builds validate those headers by SHA and do not
  regenerate them from the retained template inputs.
- Delete: `scripts/dummy-tools/`. These fake compiler/linker helpers are for
  manual Kconfig/toolchain probing and are not part of the fixed validation
  build.
- Keep: `scripts/basic/`, `scripts/kconfig/`, `scripts/mod/`,
  `scripts/dtc/`, `scripts/include/`, `scripts/Kbuild.include`,
  `scripts/Kconfig.include`, `scripts/Makefile*` files active in the fixed
  build, `link-vmlinux.sh`, `kallsyms.c`, `sorttable.c`, `elf-parse.*`,
  syscall header generation, module linker/modpost support, DTC/libfdt support,
  and version/toolchain probe scripts referenced by Makefile/Kconfig.
- Defer: disabled-config helper files such as sanitizer, rust, BTF, signing,
  ASN.1, headers-install, userprog, and optional packaging/build-maintenance
  helpers. They require a separate config-symbol and build-goal reduction proof
  before deletion.

Patch class:
- `pure_delete`

Planned verification:
- `find scripts -type d -empty -print | sort`
- `git diff --check -- ':!/.codex-qemu-kernels'`
- `JOBS=32 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh`
- `git show --stat --oneline --decorate HEAD`
- `git show --check --oneline HEAD`

Post-commit review:
- Commit: `2e58c2ba9bd6 refactor(裁剪scripts): 删除未用atomic输入和dummy工具`
- `find scripts -type d -empty -print | sort`: no empty source directories
  left.
- `git diff --check -- ':!/.codex-qemu-kernels'`: clean before commit.
- `git show --check --oneline HEAD`: clean after commit.
- `git status --porcelain=v1 -uall -- ':!/.codex-qemu-kernels'`: clean after
  commit.
- Validation: `JOBS=32 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh`
- x86_64 QEMU log:
  `.codex-qemu-kernels/logs/qemu/minix-storage-x86_64-20260505-210701-attempt1.log`
- arm64 QEMU log:
  `.codex-qemu-kernels/logs/qemu/minix-storage-arm64-20260505-210704-attempt1.log`
- Metrics: `.codex-qemu-kernels/metrics/metrics-20260505-210705.txt`

Review result:
- Clean. The fixed x86_64 and arm64 builds both completed from clean `O=`
  directories. Both QEMU Minix storage tests reported `CODEX_MINIX_TEST_PASS`
  for `/dev/nvme0n1` and `/dev/sda`.
