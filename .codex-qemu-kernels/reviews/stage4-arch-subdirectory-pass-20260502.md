# Stage 4 arch subdirectory deletion pass review

Date: 2026-05-02

Patch class: delete_plus_build_wiring

Commit: `d0ea7e33d1bc` (`arch: remove unused x86 and arm64 subdirectories`)

Scope:

- Removed x86 KVM, Xen, Hyper-V, CoCo, UML, math emulation, video, and
  unused platform-specific subdirectories.
- Removed arm64 KVM, Xen, Hyper-V, arch crypto, and board DTS subdirectories.
- Removed non-validation config fragments, keeping only
  `arch/x86/configs/x86_64_defconfig` and `arch/arm64/configs/defconfig`.

Mechanical fallout:

- Removed stale Kbuild and Makefile entries for deleted arch subtrees.
- Removed stale Kconfig entries for deleted x86 platform, PVH, math emulation,
  KVM/Xen source, and arm64 KVM/Xen support.
- Removed dead arm64 defconfig requests for deleted Xen and arch crypto code.
- Removed the direct x86 Xen head include from `arch/x86/kernel/head_64.S`.

Validation:

- `git diff --check`: passed before validation.
- First validation failed on deleted `arch/x86/xen/xen-head.S` include; the
  stale include was removed.
- `JOBS=32 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh`:
  passed after the include cleanup.
- `git show --check --oneline HEAD`: passed after commit.
- x86_64 QEMU Minix storage log:
  `.codex-qemu-kernels/logs/qemu/minix-storage-x86_64-20260502-210127-attempt1.log`
- arm64 QEMU Minix storage log:
  `.codex-qemu-kernels/logs/qemu/minix-storage-arm64-20260502-210130-attempt1.log`
- Metrics: `.codex-qemu-kernels/metrics/metrics-20260502-210131.txt`

Findings:

- No validation regression found for the fixed NVMe plus SCSI HDD Minix
  contract.
- The arch subdirectory deletion pass is committed as `d0ea7e33d1bc`.
- Remaining arch whole-directory candidates are config-backed and should be
  handled only after disabling their current config users: x86 events, ia32,
  purgatory, power, arm64 vdso32, and arm64 net.
- Temporary/non-config fallout is recorded in the compatibility ledger for
  later rollback review.
