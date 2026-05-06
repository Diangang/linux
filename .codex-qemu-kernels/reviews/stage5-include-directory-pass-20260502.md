# Stage 5 include directory pass

Date: 2026-05-02

Scope:

- Remove unused header directories under `include/` that are outside the fixed
  x86_64/arm64 QEMU storage contract.
- Keep `include/kunit/`, `include/xen/`, `include/hyperv/`, `include/kvm/`,
  and other include trees that still have current source/config users for later
  dependency-backed passes.

Removed directories:

- `include/math-emu/`
- `include/ufs/`
- `include/linux/rtc/`
- `include/linux/avf/`
- `include/linux/bnge/`
- `include/linux/bnxt/`
- `include/linux/pds/`
- `include/linux/qat/`
- `include/linux/lockd/`
- `include/uapi/linux/cifs/`
- `include/uapi/linux/nfsd/`

Dependency notes:

- `include/math-emu/` and `include/ufs/` only had self-contained include
  references in the deleted directories.
- No external explicit include users were found for the removed RTC, vendor NIC,
  QAT, lockd, CIFS, or NFSD header directories before deletion.
- The fixed validation path uses NVMe host, SCSI disk, Minix, PCI/interrupt,
  serial console, and initramfs paths; none of the removed header families are
  required by that contract.

Patch class: `pure_delete`

Temporary compatibility ledger:

- No non-config source compatibility code was added or modified in this pass.

Validation:

- `JOBS=32 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh`
- x86_64 QEMU log:
  `.codex-qemu-kernels/logs/qemu/minix-storage-x86_64-20260502-214504-attempt1.log`
- arm64 QEMU log:
  `.codex-qemu-kernels/logs/qemu/minix-storage-arm64-20260502-214507-attempt1.log`
- Metrics:
  `.codex-qemu-kernels/metrics/metrics-20260502-214508.txt`

Result:

- x86_64 build and QEMU Minix NVMe/SCSI HDD boot test passed.
- arm64 build and QEMU Minix NVMe/SCSI HDD boot test passed.
