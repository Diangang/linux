You are running the recoverable long task `linux-minimal-core-long-run`.

Use the linux-storage and karpathy-guidelines skills.

Apply karpathy-guidelines to all code-changing decisions: state assumptions
before selecting a target, prefer coarse directory and whole-file deletion
before file-internal trimming, avoid speculative cleanup or drive-by
refactoring, touch only lines directly required by the selected removal, and
define verification before looping.

Read these files before acting:

- `.codex-qemu-kernels/state.json`
- `.codex-qemu-kernels/Kernel-Minimal-Roadmap.md`
- `.codex-qemu-kernels/QEMU_KERNEL_MINIX_TEST.md`

Task goal:

Trim this Linux source tree by deleting unnecessary source code while preserving
the fixed QEMU storage contract: x86_64 and arm64 must build and boot, and both
QEMU NVMe (`/dev/nvme0n1`) and HDD-like SCSI (`/dev/sda`) disks must be
formatted as Minix, mounted, written, read, synced, and unmounted.

Validation environment:

- Boot QEMU with a generated initramfs and `rdinit=/init`.
- Do not use Debian qcow2 root filesystems.
- Do not use virtio-blk as a root-device validation dependency.
- Host-side setup may recreate and format disposable raw NVMe/SCSI HDD images
  with `mkfs.minix -3`; guest initramfs must mount and exercise both devices.

Architecture scope:

- Only x86_64 and arm64 are in scope.
- Other architectures are not required validation targets.
- Do not let unrelated other-architecture breakage block the task unless the
  changed files are shared with x86_64 or arm64.

Workspace and commit policy:

- All newly created task control/test/log/review/config/image files must stay
  under `.codex-qemu-kernels/`.
- Do not commit `.codex-qemu-kernels/`.
- Commit only kernel source changes.

Source modification policy:

- This task is deletion-first.
- Deletion order is coarse-first: whole directories first, whole files second,
  file-internal trimming last.
- Stage selection must use the concrete directory lists in
  `.codex-qemu-kernels/state.json` and
  `.codex-qemu-kernels/Kernel-Minimal-Roadmap.md`, including root-level
  directories. Do not invent abstract stages such as "non-storage subsystem"
  when an actual directory stage is available.
- Do not start file-internal minimization while a safe whole directory or whole
  source-file deletion target remains.
- Allowed source patch classes are `pure_delete`,
  `delete_plus_build_wiring`, and `move_only`.
- Do not change existing runtime logic to make trimming pass.
- Do not add stubs that fake removed behavior.
- Do not alter function semantics, control flow, error handling, locking,
  reference counting, I/O behavior, mount/read/write behavior, syscall/UAPI
  behavior, or device names.
- When all tracked files under a removed feature directory are deleted, remove
  the now-empty directory from the working tree as part of the same workflow
  step. Empty source directories are not acceptable recovery leftovers even
  though Git does not track them.
- If such a logic change appears necessary, update
  `.codex-qemu-kernels/state.json` with
  `run_control.stop_condition="logic_change_required"` and stop for human
  review.

Commit granularity:

- One source commit should remove one coarse, coherent deletion unit: a whole
  feature directory, subsystem directory slice, or group of whole files from one
  feature family.
- Tightly coupled directories/files may be removed together when needed to keep
  the tree buildable, but do not combine unrelated subsystem areas in one
  commit.
- Do not leave a feature half removed; include its dead Kconfig/Makefile/include
  wiring cleanup in the same commit.
- The commit message must clearly name the removed module/feature, state why it
  is outside the fixed validation contract, and mention x86_64 plus arm64
  validation.

Required normal-round flow:

1. Inspect state, roadmap, and current worktree status.
2. Select the largest safe directory-level deletion target available. If none
   remains in the current broad area, select the largest safe whole-file target.
   Select file-internal trimming only after directory and whole-file deletion
   opportunities are exhausted. Use the current concrete stage order:
   `fs/`, then `drivers/`, then `arch/`, then root-level directories, then
   whole-file stragglers, then file-internal surface reduction.
3. Write a dependency ledger/review note under `.codex-qemu-kernels/reviews/`.
4. Make source changes only when the target is clearly safe under the deletion
   policy.
5. Classify the patch as `pure_delete`, `delete_plus_build_wiring`,
   `move_only`, or `logic_change`.
6. Remove any source directories left empty by the selected deletion and verify
   the target directory is gone or non-empty for an intentional reason.
7. Run required x86_64 and arm64 validation. If Stage 1 automation is not
   complete, complete it before deleting kernel source.
8. Update `.codex-qemu-kernels/state.json`.
9. Commit kernel source changes only, with a clear module/feature removal
   message.
10. Review the commit using git show/stat/check and record the review under
   `.codex-qemu-kernels/reviews/`.
11. Continue to the next narrow trimming target unless an allowed stop
    condition is hit.

Allowed stop conditions:

- `validation_failure`
- `review_findings`
- `logic_change_required`
- `dependency_unclear`
- `core_functionality_regression`
- `destructive_operation`
- `dirty_worktree_conflict`
- `no_safe_next_patch`
- `needs_human_decision`
- `budget_exhausted`
- `codex_process_failed`

Clean build, clean QEMU validation, clean review, clean commit, stage
transition, and turn boundary are checkpoints only. They are not reasons to
stop.
