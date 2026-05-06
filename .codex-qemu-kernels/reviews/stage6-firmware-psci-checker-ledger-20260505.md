# Stage 6 firmware PSCI checker ledger

Target: `drivers/firmware/psci/`

Assumptions:
- Fixed validation scope is x86_64 plus arm64 QEMU boot with NVMe
  `/dev/nvme0n1`, SCSI HDD `/dev/sda`, and Minix
  mount/write/read/sync/umount.
- PSCI runtime firmware support is kept for arm64 boot.
- The PSCI checker self-test is not part of the fixed boot or storage
  contract.

Evidence:
- Fixed arm64 builds `drivers/firmware/psci/psci.o` but does not build
  `psci_checker.o`.
- `ARM_PSCI_CHECKER` appears only in `drivers/firmware/psci/Kconfig` and
  `drivers/firmware/psci/Makefile`; the fixed configs do not enable it.
- `psci_checker.c` registers a late init self-test for hotplug and suspend
  behavior, not storage discovery or Minix I/O.

Delete bucket:
- `drivers/firmware/psci/psci_checker.c`

Keep/defer:
- Keep `drivers/firmware/psci/psci.c` and `CONFIG_ARM_PSCI_FW`.
- Defer SCMI optional transport/debug files to a separate proof bucket because
  SCMI core is built in the fixed arm64 configuration.

Patch class:
- `delete_plus_build_wiring`

Planned verification:
- `find drivers/firmware -type d -empty -print | sort`
- `git diff --check -- ':!/.codex-qemu-kernels'`
- `JOBS=32 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh`
- `git show --stat --oneline --decorate HEAD`
- `git show --check --oneline HEAD`

Pre-commit verification:
- Empty source directory check was clean.
- `git diff --check -- ':!/.codex-qemu-kernels'` was clean.
- Fixed storage validation passed with:
  - x86_64 log:
    `.codex-qemu-kernels/logs/qemu/minix-storage-x86_64-20260505-212455-attempt1.log`
  - arm64 log:
    `.codex-qemu-kernels/logs/qemu/minix-storage-arm64-20260505-212458-attempt1.log`
  - metrics:
    `.codex-qemu-kernels/metrics/metrics-20260505-212459.txt`
- Both QEMU guests reached `CODEX_MINIX_TEST_PASS`.

Post-commit review:
- Commit: `6dba022ee751 refactor(裁剪firmware): 删除PSCI checker自检`
- `git show --stat --oneline --decorate HEAD` showed 3 source files changed
  with 502 deletions.
- `git show --check --oneline HEAD` was clean.
- `git show --numstat --oneline HEAD` confirmed zero insertions.
- `git status --porcelain=v1 -uall -- ':!/.codex-qemu-kernels'` was clean.
- Empty source directory check was clean after commit.
- Review result: clean.
