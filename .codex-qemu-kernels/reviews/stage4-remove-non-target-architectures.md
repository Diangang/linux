# Stage 4: remove non-target architectures

Date: 2026-05-02

Scope:

- Delete every `arch/*/` directory outside the fixed validation scope.
- Keep `arch/x86/` and `arch/arm64/`.

Removed directories:

- `arch/alpha/`
- `arch/arc/`
- `arch/arm/`
- `arch/csky/`
- `arch/hexagon/`
- `arch/loongarch/`
- `arch/m68k/`
- `arch/microblaze/`
- `arch/mips/`
- `arch/nios2/`
- `arch/openrisc/`
- `arch/parisc/`
- `arch/powerpc/`
- `arch/riscv/`
- `arch/s390/`
- `arch/sh/`
- `arch/sparc/`
- `arch/um/`
- `arch/xtensa/`

Dependency fallout:

- Top-level `Makefile` no longer wires `all` to full `dtbs` for
  `CONFIG_OF_EARLY_FLATTREE`; default arm64 image builds no longer require
  deleted 32-bit ARM DTS include trees. Explicit `dtbs` remains separate.
- `arch/arm64/configs/defconfig` disables Xen because arm64 Xen reused deleted
  `arch/arm/xen/` objects.

Validation:

- `JOBS=32 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh`
- x86_64 build passed.
- arm64 build passed.
- x86_64 QEMU Minix NVMe and SCSI test passed:
  `.codex-qemu-kernels/logs/qemu/minix-storage-x86_64-20260502-191001-attempt1.log`
- arm64 QEMU Minix NVMe and SCSI test passed:
  `.codex-qemu-kernels/logs/qemu/minix-storage-arm64-20260502-191004-attempt1.log`
- Metrics:
  `.codex-qemu-kernels/metrics/metrics-20260502-191006.txt`
