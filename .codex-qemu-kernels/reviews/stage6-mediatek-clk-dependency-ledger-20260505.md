# Stage 6 Mediatek clock dependency ledger

Target: `drivers/clk/mediatek/`

Assumptions:

- Fixed validation requires x86_64 and arm64 QEMU boot plus NVMe and
  virtio-scsi/SCSI disk Minix I/O. Mediatek SoC clock controllers are outside
  that contract.
- This patch may delete the Mediatek clock provider directory and direct
  `drivers/clk` Kconfig/Makefile wiring only. It must not change common clock
  runtime logic or other Mediatek-adjacent subsystems.
- `include/dt-bindings/` headers are separate ABI-like include surface and are
  not deleted in this drivers leaf pass.

Evidence:

- `drivers/clk/mediatek/` has 220 tracked files and is the largest remaining
  `drivers/clk` vendor leaf.
- The fixed arm64 config has `# CONFIG_ARCH_MEDIATEK is not set`; the fixed
  x86_64 config has no Mediatek clock enablement.
- Config scans found no enabled `CONFIG_COMMON_CLK_MEDIATEK` or
  `CONFIG_COMMON_CLK_MT*` entries in the fixed x86_64 or arm64 configs.
- `.cmd` scans found only empty archive/order edges for
  `drivers/clk/mediatek/`:
  `built-in.a` is created with no objects and `modules.order` is empty on both
  architectures. No Mediatek clock `.o` file is compiled.
- The parent wiring is direct and mechanical:
  `drivers/clk/Makefile` has `obj-y += mediatek/`, and
  `drivers/clk/Kconfig` sources `drivers/clk/mediatek/Kconfig`.
- External source references to Mediatek clock config symbols are absent
  outside the target directory. Remaining `ARCH_MEDIATEK` references are
  unrelated disabled platform-driver Kconfig dependencies and are not build
  users of `drivers/clk/mediatek/`.

Decision:

- Delete `drivers/clk/mediatek/` as a whole vendor clock-provider leaf.
- Delete only the parent `drivers/clk` Makefile and Kconfig wiring required to
  avoid stale references to the removed directory.

Patch class: `delete_plus_build_wiring`

Verification before looping:

- Verify `drivers/clk/mediatek/` is gone from the working tree.
- Run `git diff --check`.
- Run `JOBS=32 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh`.
- Review the commit with `git show --stat --oneline --decorate HEAD`,
  `git show --check --oneline HEAD`, and `git show -- drivers/clk`.
