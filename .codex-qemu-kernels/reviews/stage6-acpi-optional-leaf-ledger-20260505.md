# Stage 6 ACPI optional leaf dependency ledger

Target: `drivers/acpi/`

Assumptions:
- Fixed validation scope is x86_64 plus arm64 QEMU boot with NVMe
  `/dev/nvme0n1`, SCSI HDD `/dev/sda`, and Minix
  mount/write/read/sync/umount.
- ACPI core, ACPICA, PCI/PNP/resource routing, x86 ACPI boot, arm64 ACPI boot,
  and arm64 APEI/GHES paths are kept because they are built or boot-adjacent.
- Delete only whole optional leaf files whose Kconfig symbols are disabled or
  whose Makefile selector is a non-config future-use hook, then remove matching
  build/config wiring.

Evidence:
- Current fixed `.cmd` inventory builds ACPI core, ACPICA, ACPI PCI/PNP,
  x86 ACPI, arm64 ACPI, ACPI NUMA, and arm64 APEI/GHES objects.
- The fixed `.cmd` inventory does not build ACPI IPMI, ACPI configfs, ACPI
  debugger user access, ACPI extlog, ACPI ADXL, ACPI PCC, ACPI watchdog,
  processor aggregator, smart battery, EC debugfs, arm64 AGDI/MPAM, APEI EINJ,
  APEI ERST debug, NVIDIA GHES, or ACPICA `ACPI_FUTURE_USAGE` objects.
- Both fixed configs have `CONFIG_DEBUG_FS` disabled; arm64 has
  `CONFIG_ACPI_AGDI` disabled and `CONFIG_ACPI_APEI_GHES_NVIDIA` /
  `CONFIG_ACPI_APEI_ERST_DEBUG` disabled; x86_64 has
  `CONFIG_ACPI_EC_DEBUGFS` and `CONFIG_ACPI_PROCESSOR_AGGREGATOR` disabled.
- These deleted leaves are outside the fixed storage contract: they are
  platform diagnostics, debug/configfs/error-injection, vendor error decode,
  laptop power/battery, or optional firmware-table features.

Delete bucket:
- `drivers/acpi/acpi_adxl.c`
- `drivers/acpi/acpi_configfs.c`
- `drivers/acpi/acpi_dbg.c`
- `drivers/acpi/acpi_extlog.c`
- `drivers/acpi/acpi_ipmi.c`
- `drivers/acpi/acpi_pad.c`
- `drivers/acpi/acpi_pcc.c`
- `drivers/acpi/acpi_watchdog.c`
- `drivers/acpi/acpica/hwtimer.c`
- `drivers/acpi/acpica/nsdumpdv.c`
- `drivers/acpi/apei/einj-core.c`
- `drivers/acpi/apei/einj-cxl.c`
- `drivers/acpi/apei/erst-dbg.c`
- `drivers/acpi/apei/ghes-nvidia.c`
- `drivers/acpi/arm64/agdi.c`
- `drivers/acpi/arm64/mpam.c`
- `drivers/acpi/ec_sys.c`
- `drivers/acpi/sbs.c`
- `drivers/acpi/sbshc.c`
- `drivers/acpi/sbshc.h`
- `drivers/acpi/viot.c`

Keep/defer:
- Keep ACPI core, ACPICA built objects, PCI/PNP/resource routing, x86 ACPI,
  arm64 ACPI table/PCI/IORT/GTDT/APMT paths, NUMA/HMAT, APEI core/GHES, and
  processor/container/memory-hotplug objects observed in fixed builds.
- Defer AC/battery/button/fan/video/thermal/NHLT/CPPC leaves even when not
  built by the fixed configs, because they have broader helper/header or
  platform-policy references and need a separate proof if removed.

Patch class:
- `delete_plus_build_wiring`

Planned verification:
- `find drivers/acpi -type d -empty -print | sort`
- `git diff --check -- ':!/.codex-qemu-kernels'`
- `JOBS=32 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh`
- `git show --stat --oneline --decorate HEAD`
- `git show --check --oneline HEAD`

Pre-commit verification:
- Empty source directory check was clean.
- `git diff --check -- ':!/.codex-qemu-kernels'` was clean.
- Fixed storage validation passed with:
  - x86_64 log:
    `.codex-qemu-kernels/logs/qemu/minix-storage-x86_64-20260505-211718-attempt1.log`
  - arm64 log:
    `.codex-qemu-kernels/logs/qemu/minix-storage-arm64-20260505-211721-attempt1.log`
  - metrics:
    `.codex-qemu-kernels/metrics/metrics-20260505-211722.txt`
- Both QEMU guests reached `CODEX_MINIX_TEST_PASS`.

Post-commit review:
- Commit: `371fcc338db9 refactor(裁剪acpi): 删除非固定目标可选叶子`
- `git show --stat --oneline --decorate HEAD` showed 42 source files changed
  with 7684 deletions.
- `git show --check --oneline HEAD` was clean.
- `git status --porcelain=v1 -uall -- ':!/.codex-qemu-kernels'` was clean.
- Empty source directory check was clean after commit.
- Review result: clean.
