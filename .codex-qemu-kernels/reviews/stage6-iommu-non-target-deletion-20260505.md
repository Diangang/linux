# Stage 6 IOMMU Non-Target Deletion Review

Commit: `6de0a3c9506e refactor(裁剪iommu): 删除非目标IOMMU驱动`

Scope:
- Removed non-target platform IOMMU drivers not used by fixed x86_64/arm64 QEMU Minix validation.
- Removed unused IOMMU page-table test/format files outside the active x86_64 and arm64 paths.
- Removed disabled debug, IOMMUFD, SVA, QCOM, Tegra241, and KUnit build wiring for deleted files.

Kept:
- IOMMU core, IOVA, DMA-IOMMU, OF IOMMU, sysfs/tracing, and x86 IRQ remapping.
- x86_64 AMD and Intel IOMMU main paths.
- generic_pt AMDv1, VTDSS, and x86_64 formats.
- arm64 ARM SMMU, SMMUv3, LPAE page-table, and NVIDIA SMMU implementation object currently built by ARM_SMMU.

Static checks:
- `find drivers/iommu -type d -empty -print | sort`: no output.
- `git diff --check -- ':!/.codex-qemu-kernels'`: no output.
- `git show --check --oneline HEAD`: no whitespace errors.

Validation:
- Command: `JOBS=32 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh`
- Result: passed.
- x86_64 QEMU log: `.codex-qemu-kernels/logs/qemu/minix-storage-x86_64-20260505-204950-attempt1.log`
- arm64 QEMU log: `.codex-qemu-kernels/logs/qemu/minix-storage-arm64-20260505-204953-attempt1.log`
- Metrics: `.codex-qemu-kernels/metrics/metrics-20260505-204954.txt`

Review result:
- Clean for the fixed validation contract.
