# stage6 firmware non-target deletion review

- Commit `9e9d7d8585fe` deletes non-target `drivers/firmware` vendor directories, unused single-file firmware protocol drivers, and i.MX SCMI vendor extensions plus direct Kconfig/Makefile wiring.
- Kept firmware paths cover EFI, DMI, firmware memmap, PSCI, SMCCC, SDEI, and SCMI core files required by the fixed x86_64 and arm64 boot/storage validation path.
- Removed empty feature directories in the same deletion step.
- Validation passed with `JOBS=32 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh`.
- QEMU logs:
  - `.codex-qemu-kernels/logs/qemu/minix-storage-x86_64-20260505-202151-attempt1.log`
  - `.codex-qemu-kernels/logs/qemu/minix-storage-arm64-20260505-202154-attempt1.log`
- Metrics: `.codex-qemu-kernels/metrics/metrics-20260505-202155.txt`
- `git show --check --oneline HEAD` passed.
