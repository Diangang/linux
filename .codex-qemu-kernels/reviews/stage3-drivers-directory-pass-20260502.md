# Stage 3 drivers directory pass review

Date: 2026-05-02

Scope:

- Removed broad `drivers/` subdirectories outside the fixed QEMU storage
  validation contract.
- Kept only `drivers/acpi/`, `drivers/amba/`, `drivers/base/`,
  `drivers/block/`, `drivers/bus/`, `drivers/char/`, `drivers/clk/`,
  `drivers/clocksource/`, `drivers/dma/`, `drivers/firmware/`,
  `drivers/gpio/`, `drivers/idle/`, `drivers/iommu/`, `drivers/irqchip/`,
  `drivers/nvme/`, `drivers/of/`, `drivers/pci/`, `drivers/pnp/`,
  `drivers/scsi/`, `drivers/tty/`, and `drivers/virtio/`.

Dependency cleanup:

- Removed stale `drivers/Kconfig` source entries and `drivers/Makefile` build
  targets for deleted driver directories.
- Gated removed-subsystem selects in kept Kconfig files so defconfig does not
  force deleted driver subsystems back on.
- Trimmed arm64 defconfig away from non-target vendor platform defaults that
  linked against deleted driver infrastructure.
- Disconnected x86 RTC wallclock helpers after deleting `drivers/rtc/`.

Temporary rollback ledger entries:

- `arch/x86/kernel/rtc.c`
- `arch/x86/kernel/x86_init.c`
- `drivers/Makefile`
- `drivers/Kconfig` and dependent kept Kconfig files

Validation:

- `JOBS=32 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh`
- Passed x86_64 build and QEMU Minix NVMe plus SCSI HDD test.
- Passed arm64 build and QEMU Minix NVMe plus SCSI HDD test.
- Metrics: `.codex-qemu-kernels/metrics/metrics-20260502-202550.txt`
