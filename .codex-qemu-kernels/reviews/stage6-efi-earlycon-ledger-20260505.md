# Stage 6 ledger: EFI earlycon

## Target

- Deletion unit: `drivers/firmware/efi/earlycon.c`
- Patch class: `delete_plus_build_wiring`
- Stage: whole-file straggler under `drivers/firmware/efi/`

## Assumptions

- The fixed QEMU contract uses serial consoles: `console=ttyS0` on x86_64 and `console=ttyAMA0` on arm64.
- EFI framebuffer earlycon (`earlycon=efifb`) is not used by the validation initramfs boot path.
- Generic serial earlycon support remains in `drivers/tty/serial/`; this patch only removes the EFI framebuffer earlycon provider.
- EFI core, EFI libstub, and runtime support remain in place.

## Dependency scan

- Build wiring:
  - `drivers/firmware/efi/Makefile` builds `earlycon.o` from `CONFIG_EFI_EARLYCON`.
  - `drivers/firmware/efi/Kconfig` defines `EFI_EARLYCON` as default-y when serial earlycon is enabled.
  - `arch/x86/configs/x86_64_defconfig` and `arch/arm64/configs/defconfig` request `CONFIG_EFI_EARLYCON=y`.
- Direct source users:
  - `drivers/firmware/efi/efi-init.c` calls `efi_earlycon_reprobe()` only under `IS_ENABLED(CONFIG_EFI_EARLYCON)`/preprocessor guards.
  - `drivers/firmware/efi/libstub/efi-stub-entry.c` tests `CONFIG_EFI_EARLYCON` as a config condition.
  - `arch/arm64/kernel/image-vars.h` exports the symbol only under `CONFIG_EFI_EARLYCON`.
- Runtime evidence:
  - The fixed QEMU logs use serial consoles and contain no `earlycon=efifb` usage.

## Planned edit

- Delete `drivers/firmware/efi/earlycon.c`.
- Remove `EFI_EARLYCON` Kconfig entry.
- Remove `earlycon.o` Makefile wiring.
- Remove `CONFIG_EFI_EARLYCON=y` from x86_64 and arm64 defconfigs.

## Verification plan

1. Scan for residual `CONFIG_EFI_EARLYCON`, `EFI_EARLYCON`, `efi_earlycon`, `earlycon=efifb`, and `earlycon.o` source references.
2. Run `git diff --check`.
3. Check `drivers/firmware/efi` has no empty directory leftovers.
4. Run `JOBS=32 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh`.

## Verification result

- Residual scan found only generic serial earlycon build objects under `drivers/tty/serial/`; no EFI earlycon references remained.
- `git diff --check` passed.
- `find drivers/firmware/efi -type d -empty -print` produced no empty directories.
- Fixed storage validation passed with generated initramfs and `rdinit=/init`:
  - x86_64 QEMU log: `.codex-qemu-kernels/logs/qemu/minix-storage-x86_64-20260505-222844-attempt1.log`
  - arm64 QEMU log: `.codex-qemu-kernels/logs/qemu/minix-storage-arm64-20260505-222847-attempt1.log`
  - metrics: `.codex-qemu-kernels/metrics/metrics-20260505-222848.txt`

## Post-commit review

- Commit: `fb31ba88764f refactor(裁剪firmware): 删除EFI earlycon支持`
- `git show --stat --oneline --decorate HEAD` matched the intended 9-file source patch: one deleted EFI earlycon source file plus direct config/build/symbol cleanup.
- `git show --check --oneline HEAD` reported no whitespace errors.
- `git show --name-status --oneline HEAD` showed no unrelated subsystem additions or runtime logic rewrites.
- `git status --short` was clean for kernel source after the commit.
