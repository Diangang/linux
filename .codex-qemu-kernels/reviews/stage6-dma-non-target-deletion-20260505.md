# stage6-dma-non-target-deletion review

Status: committed_clean

Commit: e2f1634a0b87 refactor(裁剪dma): 删除非目标DMA驱动

Scope:
- Deleted non-target platform DMA controller drivers under drivers/dma/.
- Removed empty helper subdirectories amd, idxd, loongson, mediatek, stm32, and ti.
- Reduced drivers/dma Kconfig and Makefile wiring to retained fixed-contract paths.
- Removed stale defconfig requests for deleted DMA symbols.

Kept paths:
- DMA core: dmaengine, virt-dma, ACPI DMA, and OF DMA helpers.
- x86_64: DW DMAC core and HSU DMA core.
- arm64: FSL eDMA, MV XOR v2, PL330, and QCOM HIDMA.

Validation:
- JOBS=32 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh
- x86_64 log: .codex-qemu-kernels/logs/qemu/minix-storage-x86_64-20260505-203902-attempt1.log
- arm64 log: .codex-qemu-kernels/logs/qemu/minix-storage-arm64-20260505-203905-attempt1.log
- metrics: .codex-qemu-kernels/metrics/metrics-20260505-203906.txt

Findings:
- Fixed x86_64 and arm64 builds completed.
- QEMU Minix storage tests passed for both architectures on attempt 1.
- The retained DMA paths cover observed fixed-build objects and storage-adjacent boot/runtime DMA paths.
- git show --check --oneline HEAD reported no whitespace errors.
