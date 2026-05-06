# Stage 6 EFI zboot ledger

Target: EFI zboot decompressor support

Selected bucket:

- `drivers/firmware/efi/libstub/Makefile.zboot`
- `drivers/firmware/efi/libstub/zboot.c`
- `drivers/firmware/efi/libstub/zboot-decompress-gzip.c`
- `drivers/firmware/efi/libstub/zboot-decompress-zstd.c`
- `drivers/firmware/efi/libstub/zboot-header.S`
- `drivers/firmware/efi/libstub/zboot.lds`
- Mechanical Kconfig, libstub Makefile, and arm64 boot target wiring for
  `CONFIG_EFI_ZBOOT`

Assumptions:

- The fixed contract is x86_64 plus arm64 QEMU boot with generated initramfs,
  NVMe `/dev/nvme0n1`, virtio-SCSI HDD `/dev/sda`, and Minix
  mount/write/read/sync/unmount.
- The fixed arm64 validation boots the raw `Image`, not `vmlinuz.efi`.
- EFI zboot is disabled in the fixed arm64 config and outside the fixed
  storage contract.

Evidence:

- `.codex-qemu-kernels/build-arm64/.config` has
  `# CONFIG_EFI_ZBOOT is not set`.
- `arch/arm64/configs/defconfig` has the same disabled symbol.
- No fixed build artifact exists for zboot sources or `Makefile.zboot`
  generated targets.
- x86_64 fixed build has no `CONFIG_EFI_ZBOOT` entry and does not use the
  arm64 `vmlinuz.efi` target.

Classification:

- `delete_plus_build_wiring`

Deferred:

- Normal EFI stub files remain untouched because x86_64 and arm64 still build
  EFI stub/libstub objects.
- Generic `EFI_SBAT_FILE` support remains for x86 EFI stub, with its dependency
  narrowed away from removed zboot.

Verification plan:

- `rg -n "EFI_ZBOOT|Makefile\\.zboot|zboot|zboot-header|zboot\\.lds|zboot-decompress" arch drivers include`
- `git diff --check`
- `find arch/arm64/boot drivers/firmware/efi/libstub -type d -empty -print`
- `JOBS=32 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh`
- `git show --stat --oneline --decorate HEAD`
- `git show --check --oneline HEAD`

Validation:

- `JOBS=32 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh`
- Result: passed for x86_64 and arm64.
- x86_64 QEMU log:
  `.codex-qemu-kernels/logs/qemu/minix-storage-x86_64-20260505-215218-attempt1.log`
- arm64 QEMU log:
  `.codex-qemu-kernels/logs/qemu/minix-storage-arm64-20260505-215221-attempt1.log`
- Metrics:
  `.codex-qemu-kernels/metrics/metrics-20260505-215222.txt`

Commit:

- `c0b5e47bb8f3 refactor(裁剪firmware): 删除EFI zboot decompressor`
- Patch class: `delete_plus_build_wiring`

Review:

- `git diff --check`: clean before commit.
- Empty directory check under `arch/arm64/boot` and
  `drivers/firmware/efi/libstub`: clean.
- Residual zboot reference check across `arch`, `drivers`, and `include`:
  clean.
- `git show --stat --oneline --decorate HEAD`: 15 files changed, 5
  insertions, 570 deletions.
- `git show --check --oneline HEAD`: clean.
- `git status --short`: clean after commit.
