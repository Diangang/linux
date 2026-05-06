# Stage 6 EFI STMM dependency ledger

Target: `drivers/firmware/efi/stmm/`

Assumptions:

- Fixed validation boots generated initramfs images on x86_64 and arm64 and
  requires only QEMU NVMe, virtio-scsi/SCSI disk, Minix, devtmpfs, and the boot
  firmware/device paths needed by those images.
- TEE-backed EFI variable storage is outside that storage contract.
- This pass may delete only the whole STMM implementation directory and its
  dead build/config wiring. No runtime EFI logic may be changed.

Evidence:

- `drivers/firmware/efi/stmm/` contains only
  `tee_stmm_efi.c` and its private `mm_communication.h`.
- `drivers/firmware/efi/Makefile` has a single build edge:
  `obj-$(CONFIG_TEE_STMM_EFI) += stmm/tee_stmm_efi.o`.
- `drivers/firmware/efi/Kconfig` defines `TEE_STMM_EFI` as depending on
  `EFI && OPTEE`.
- Current fixed configs contain `CONFIG_EFI=y` but no enabled
  `CONFIG_TEE_STMM_EFI` or `CONFIG_OPTEE` entry.
- `.cmd` dependency scan across both fixed build directories found no
  `drivers/firmware/efi/stmm`, `tee_stmm`, or `mm_communication` dependency.
- Tree reference scan found only the STMM source itself plus the Makefile and
  Kconfig entries; unrelated `stmmac` and x86 VC communication hits are not
  users of this EFI STMM directory.

Decision:

- Delete `drivers/firmware/efi/stmm/` as a whole feature leaf.
- Delete the `TEE_STMM_EFI` Kconfig entry and its Makefile object edge in the
  same patch so the removed directory is not left half wired.

Patch class: `delete_plus_build_wiring`

Verification before looping:

- Verify `drivers/firmware/efi/stmm/` is gone or non-empty only for an
  intentional reason.
- Run `JOBS=32 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh`.
- Review with `git show --stat --oneline --decorate HEAD`,
  `git show --check --oneline HEAD`, and `git show -- drivers/firmware/efi`.
