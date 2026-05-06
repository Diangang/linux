# Stage 6 irqchip non-target deletion review

Commit: 296296eeb810 refactor(裁剪irqchip): 删除非目标中断控制器驱动

Scope:
- Deleted non-target `drivers/irqchip` controller implementations outside the fixed x86_64 and arm64 QEMU storage contract.
- Kept x86_64 `irq-msi-lib` and arm64 GIC/GICv2m/GICv3/GICv4/GICv5/ITS/Xilinx INTC paths observed in fixed builds/configs.
- Reduced `drivers/irqchip/Kconfig` and `drivers/irqchip/Makefile` to retained symbols/objects.
- Removed stale `CONFIG_AL_FIC` arm64 defconfig request after deleting the implementation.

Checks:
- `find drivers/irqchip -type d -empty -print | sort`: no empty source directories left.
- `git diff --check -- ':!/.codex-qemu-kernels'`: clean before commit.
- `git show --check --oneline HEAD`: clean after commit.
- `git status --porcelain=v1 -uall -- ':!/.codex-qemu-kernels'`: clean after commit.

Validation:
- `JOBS=32 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh`
- x86_64 QEMU log: `.codex-qemu-kernels/logs/qemu/minix-storage-x86_64-20260505-205621-attempt1.log`
- arm64 QEMU log: `.codex-qemu-kernels/logs/qemu/minix-storage-arm64-20260505-205624-attempt1.log`
- metrics: `.codex-qemu-kernels/metrics/metrics-20260505-205625.txt`

Review result:
- Clean. The fixed x86_64 path still builds `irq-msi-lib`; the fixed arm64 path boots through GIC and passes NVMe plus SCSI HDD Minix I/O.
