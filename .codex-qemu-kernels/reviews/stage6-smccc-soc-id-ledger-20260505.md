# Stage 6 ledger: SMCCC SOC_ID

## Target

- Deletion unit: `drivers/firmware/smccc/soc_id.c`
- Patch class: `delete_plus_build_wiring`
- Stage: whole-file straggler under `drivers/firmware/`

## Assumptions

- The fixed validation contract needs arm64 PSCI/SMCCC core firmware calls for boot and timer/platform setup, so `smccc.c`, `kvm_guest.c`, and PSCI stay in place.
- The removed file only registers an optional SoC bus device and exposes SMCCC SOC_ID information through sysfs.
- `arm_smccc_get_soc_id_version()` and `arm_smccc_get_soc_id_revision()` stay in `smccc.c` because irqchip and ACPI code can still query the cached firmware values.
- x86_64 does not build this file; arm64 currently builds it through `CONFIG_ARM_SMCCC_SOC_ID=y`.

## Dependency scan

- Build wiring:
  - `drivers/firmware/smccc/Makefile` builds `soc_id.o` only from `CONFIG_ARM_SMCCC_SOC_ID`.
  - `drivers/firmware/smccc/Kconfig` defines `ARM_SMCCC_SOC_ID` as a bool depending on `HAVE_ARM_SMCCC_DISCOVERY`, default y, selecting `SOC_BUS`.
  - `arch/arm64/configs/defconfig` enables `CONFIG_ARM_SMCCC_SOC_ID=y`.
- Direct users:
  - `drivers/firmware/smccc/soc_id.c` owns `smccc_soc_init()` and the `soc_device_register()` call.
  - No source outside this file calls `smccc_soc_init()`.
- Preserved users:
  - `drivers/irqchip/irq-gic-v3.c` and `drivers/acpi/arm64/thermal_cpufreq.c` call `arm_smccc_get_soc_id_version()`.
  - Those accessors are preserved in `drivers/firmware/smccc/smccc.c`.

## Planned edit

- Delete `drivers/firmware/smccc/soc_id.c`.
- Remove `ARM_SMCCC_SOC_ID` Kconfig entry.
- Remove the `soc_id.o` Makefile line.
- Remove `CONFIG_ARM_SMCCC_SOC_ID=y` from the arm64 defconfig.

## Verification plan

1. Scan for residual `ARM_SMCCC_SOC_ID`, `soc_id.c`, and `soc_id.o` source references.
2. Run `git diff --check`.
3. Check `drivers/firmware/smccc` has no accidental empty-directory leftover.
4. Run `JOBS=32 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh`.

## Verification result

- Residual source scan for `CONFIG_ARM_SMCCC_SOC_ID`, `ARM_SMCCC_SOC_ID`, `soc_id.o`, `drivers/firmware/smccc/soc_id.c`, `smccc_soc_init`, and `soc_device_register()` found only generic SoC bus declarations/users outside the removed SMCCC file.
- `git diff --check` passed before commit.
- `find drivers/firmware/smccc -type d -empty -print` produced no output.
- Fixed storage validation passed:
  - x86_64: `.codex-qemu-kernels/logs/qemu/minix-storage-x86_64-20260505-222048-attempt1.log`
  - arm64: `.codex-qemu-kernels/logs/qemu/minix-storage-arm64-20260505-222051-attempt1.log`
  - metrics: `.codex-qemu-kernels/metrics/metrics-20260505-222052.txt`

## Commit review

- Commit: `e25ffb33664d refactor(裁剪firmware): 删除SMCCC SOC_ID设备支持`
- `git show --stat --oneline --decorate HEAD` showed 4 files changed and 186 deletions.
- `git show --check --oneline HEAD` passed.
- Review status: clean.
