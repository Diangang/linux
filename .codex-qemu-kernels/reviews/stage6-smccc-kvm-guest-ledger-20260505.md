# Stage 6 firmware ledger: SMCCC KVM guest services

## Target

- Deletion unit: Arm SMCCC KVM guest hypervisor service discovery
- Source file: `drivers/firmware/smccc/kvm_guest.c`
- Patch class: `delete_plus_build_wiring`
- Stage: whole-file straggler under `drivers/firmware/smccc/`

## Assumptions

- The fixed validation contract uses x86_64 and arm64 QEMU storage boot, generated initramfs, and Minix I/O on NVMe plus SCSI.
- The arm64 validation boot uses `qemu-system-aarch64 -M virt -cpu cortex-a57` and does not require KVM hypervisor vendor services.
- Generic SMCCC discovery, PSCI boot, SMCCC TRNG probing, and arch mitigation calls remain intact through `drivers/firmware/smccc/smccc.c` and PSCI.
- Removing KVM-specific SMCCC service discovery does not alter NVMe, SCSI, block, VFS, Minix, device names, or serial console behavior.

## Dependency scan

- Build wiring:
  - `drivers/firmware/smccc/Makefile` builds `kvm_guest.o` with `smccc.o` under `CONFIG_HAVE_ARM_SMCCC_DISCOVERY`.
- Direct source users:
  - `drivers/firmware/psci/psci.c` calls `kvm_init_hyp_services()` during SMCCC/PSCI init.
  - `arch/arm64/kernel/cpufeature.c` calls `kvm_arm_target_impl_cpu_init()` for KVM-provided target implementation CPU info.
  - `arch/arm64/include/asm/hypervisor.h` declares the KVM guest service helpers.
- Runtime evidence:
  - Fixed arm64 logs show PSCI boot and storage devices, with no KVM service dependency in the serial validation path.

## Planned edit

- Delete `drivers/firmware/smccc/kvm_guest.c`.
- Remove `kvm_guest.o` from SMCCC build wiring.
- Remove direct KVM guest service init calls and declarations.
- Leave PSCI, generic SMCCC discovery, TRNG, arch workaround, and storage paths intact.

## Verification plan

1. Scan for residual `kvm_guest.o`, deleted file path, `kvm_init_hyp_services`, `kvm_arm_hyp_service_available`, and `kvm_arm_target_impl_cpu_init` references.
2. Run `git diff --check`.
3. Check `drivers/firmware/smccc` has no empty directory leftovers.
4. Run `JOBS=32 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh`.

## Verification result

- Residual scan after edit: clean for `kvm_guest.o`, deleted file path, `kvm_init_hyp_services`, `kvm_arm_hyp_service_available`, and `kvm_arm_target_impl_cpu_init`.
- `git diff --check`: clean.
- Empty directory check: `drivers/firmware/smccc` is not empty and has no empty directory leftover.
- Fixed storage contract: passed with `JOBS=32 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh`.
  - x86_64 log: `.codex-qemu-kernels/logs/qemu/minix-storage-x86_64-20260505-225111-attempt1.log`
  - arm64 log: `.codex-qemu-kernels/logs/qemu/minix-storage-arm64-20260505-225114-attempt1.log`
  - metrics: `.codex-qemu-kernels/metrics/metrics-20260505-225116.txt`

## Post-commit review

- Commit: `bd173f022734 refactor(裁剪firmware): 删除SMCCC KVM guest服务`
- `git show --stat --oneline --decorate HEAD`: expected five-file patch, deleting `drivers/firmware/smccc/kvm_guest.c` and only direct arm64/PSCI/SMCCC wiring.
- `git show --check --oneline HEAD`: clean.
- `git show --name-status --oneline HEAD`: one deleted source file and four direct wiring edits.
- `git status --short`: clean after committing kernel source changes only.
- Review result: clean.
