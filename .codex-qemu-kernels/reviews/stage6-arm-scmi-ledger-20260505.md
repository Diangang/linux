# Stage 6 ledger: ARM SCMI

## Target

- Deletion unit: `drivers/firmware/arm_scmi/`
- Tightly coupled fallout:
  - `drivers/clk/clk-scmi.c`
  - SCMI Kconfig/Makefile/defconfig/select wiring
  - SCMI public headers with no remaining source users
  - SCMI trace event header
- Patch class: `delete_plus_build_wiring`
- Stage: whole-file/support-tree straggler under `drivers/firmware/`

## Assumptions

- The fixed QEMU storage contract does not provide or require an SCMI firmware device.
- arm64 fixed validation needs PSCI, GIC/ITS, PCI, NVMe, virtio-scsi, Minix, initramfs, and serial console; it does not need SCMI protocol enumeration.
- SCMI currently appears in the arm64 QEMU log only as `scmi_core: SCMI protocol bus registered`, with no SCMI-backed storage or boot device.
- Removing `drivers/clk/clk-scmi.c` is mechanical fallout from deleting SCMI; the fixed QEMU arm64 clock path uses the architectural timer and does not depend on SCMI clocks.
- Public SCMI headers are removed only after source scans show no users outside the SCMI stack, the SCMI clock client, and an IMX SCMI header that is itself unused by remaining source.

## Dependency scan

- `drivers/firmware/Makefile` always descends into `arm_scmi/`; `drivers/firmware/Kconfig` sources `drivers/firmware/arm_scmi/Kconfig`.
- `arch/arm64/configs/defconfig` enables the SCMI protocol, SMC transport, quirks, and `COMMON_CLK_SCMI`.
- `arch/arm64/Kconfig.platforms` selects SCMI symbols for `ARCH_STM32`; the fixed QEMU arm64 target is `virt`, not STM32.
- `drivers/clk/Kconfig` and `drivers/clk/Makefile` define/build only the SCMI clock client.
- Source users outside the SCMI tree:
  - `drivers/clk/clk-scmi.c` includes `linux/scmi_protocol.h`.
  - `include/linux/scmi_imx_protocol.h` includes `linux/scmi_protocol.h`.
  - `include/linux/firmware/imx/sm.h` includes `linux/scmi_imx_protocol.h`.
  - No other source includes `linux/scmi_protocol.h`, `linux/scmi_imx_protocol.h`, or `trace/events/scmi.h`.

## Planned edit

- Delete `drivers/firmware/arm_scmi/` recursively.
- Remove firmware Kconfig and Makefile entries for ARM SCMI.
- Delete `drivers/clk/clk-scmi.c` and remove its Kconfig/Makefile entries.
- Remove SCMI selects from the STM32 arm64 platform Kconfig entry.
- Remove SCMI and SCMI clock config requests from `arch/arm64/configs/defconfig`.
- Delete unused SCMI headers:
  - `include/linux/scmi_protocol.h`
  - `include/linux/scmi_imx_protocol.h`
  - `include/linux/firmware/imx/sm.h`
  - `include/trace/events/scmi.h`

## Verification plan

1. Scan source for residual `ARM_SCMI`, `COMMON_CLK_SCMI`, `scmi_protocol.h`, `scmi_imx_protocol.h`, `trace/events/scmi`, `clk-scmi`, and `drivers/firmware/arm_scmi`.
2. Run `git diff --check`.
3. Verify removed feature directories are gone or intentionally non-empty.
4. Run `JOBS=32 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh`.

## Verification result

- Residual source scan for SCMI config symbols, headers, trace header, `clk-scmi`, and `drivers/firmware/arm_scmi` produced no matches.
- `git diff --check` passed before commit.
- Empty-directory scan over `drivers/firmware`, `drivers/clk`, `include/linux/firmware/imx`, and `include/trace/events` produced no output.
- Fixed storage validation passed:
  - x86_64: `.codex-qemu-kernels/logs/qemu/minix-storage-x86_64-20260505-222457-attempt1.log`
  - arm64: `.codex-qemu-kernels/logs/qemu/minix-storage-arm64-20260505-222500-attempt1.log`
  - metrics: `.codex-qemu-kernels/metrics/metrics-20260505-222501.txt`
- Post-delete QEMU log scan for `scmi`/`SCMI` in the new x86_64 and arm64 logs produced no matches.

## Commit review

- Commit: `705a4808d321 refactor(裁剪firmware): 删除ARM SCMI协议栈`
- `git show --stat --oneline --decorate HEAD` showed 37 files changed and 18939 deletions.
- `git show --check --oneline HEAD` passed.
- Review status: clean.
