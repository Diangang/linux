# Stage 5 whole-tree coarse directory scan review

Date: 2026-05-02

## Scope

Removed coarse directory targets outside the fixed x86_64/arm64 QEMU NVMe plus
SCSI HDD Minix validation contract:

- `fs/cramfs/`
- root `virt/`
- `drivers/base/test/`
- `drivers/bus/fsl-mc/` and `drivers/bus/mhi/`
- `drivers/acpi/dptf/`, `drivers/acpi/nfit/`, `drivers/acpi/pmic/`, and
  `drivers/acpi/riscv/`
- `drivers/pci/switch/`
- `drivers/iommu/iommufd/` and `drivers/iommu/riscv/`
- non-QEMU SCSI HBA, transport, and service directories under `drivers/scsi/`

## Mechanical fallout

Removed stale Kconfig, Makefile, and Kbuild wiring from:

- `Kbuild`
- `fs/Kconfig` and `fs/Makefile`
- `drivers/base/Kconfig` and `drivers/base/Makefile`
- `drivers/bus/Kconfig` and `drivers/bus/Makefile`
- `drivers/acpi/Kconfig` and `drivers/acpi/Makefile`
- `drivers/pci/Kconfig` and `drivers/pci/Makefile`
- `drivers/iommu/Kconfig` and `drivers/iommu/Makefile`
- `drivers/scsi/Kconfig` and `drivers/scsi/Makefile`
- `arch/arm64/configs/defconfig`

## Validation

Command:

```sh
JOBS=32 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh
```

Result: passed.

Logs:

- `.codex-qemu-kernels/logs/qemu/minix-storage-x86_64-20260502-215958-attempt1.log`
- `.codex-qemu-kernels/logs/qemu/minix-storage-arm64-20260502-220001-attempt1.log`
- `.codex-qemu-kernels/metrics/metrics-20260502-220002.txt`

## Temporary compatibility ledger

No new non-config source compatibility code was added in this pass. The patch is
directory deletion plus Kconfig, Makefile, Kbuild, and defconfig cleanup only.

## Review notes

- x86_64 and arm64 fixed initramfs storage validation both passed on attempt 1.
- The kept validation path still includes NVMe host, SCSI core, SCSI disk,
  virtio-scsi, PCI, interrupt, initramfs, Minix, and serial console support.
- `Documentation/` remains explicitly untouched and retained as project
  reference material.
