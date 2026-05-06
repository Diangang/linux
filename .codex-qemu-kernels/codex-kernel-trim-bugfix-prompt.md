You are in bugfix mode for `linux-minimal-core-long-run`.

Use the workflow skill as the single entrypoint.

Apply workflow routing during bugfix work: surface uncertainty instead of
guessing, make the smallest mechanical fix that addresses the observed fallout,
do not broaden scope into unrelated cleanup, and verify the fix with explicit
checks before returning to normal continuation. Deep-read workflow-managed
external references only when the selected bugfix needs them.

Read:

- `.codex-qemu-kernels/state.json`
- `.codex-qemu-kernels/Kernel-Minimal-Roadmap.md`
- the latest `.codex-qemu-kernels/logs/agent-runs/` log
- the latest `.codex-qemu-kernels/reviews/` note, if present

Bugfix scope:

You may fix only mechanical fallout from deletion:

- stale Kconfig references;
- stale Makefile targets;
- stale includes or declarations;
- unused variable/function fallout caused by deletion;
- empty source directories left behind after all files in a removed feature
  directory were deleted;
- missing review/state updates under `.codex-qemu-kernels/`.

The normal task is coarse-first: directory deletion, then whole-file deletion,
then file-internal trimming only after those opportunities are exhausted.
Bugfix work must not broaden a coarse deletion into unrelated file-internal
minimization.

You must not fix failures by changing existing runtime logic.

Validation must remain initramfs-based. Do not reintroduce Debian qcow2 root
filesystems or virtio-blk root-device dependencies to make tests pass.

If the failure requires changing function bodies, control flow, data structure
semantics, error handling, locking, reference counting, I/O behavior,
mount/read/write behavior, syscall/UAPI behavior, device names, or adding stubs,
set `run_control.stop_condition="logic_change_required"` in
`.codex-qemu-kernels/state.json` and stop for human review.

After a mechanical bugfix:

1. Re-run required validation or the failing subset plus any affected fixed QEMU
   storage test. Accepted source commits still require x86_64 and arm64
   validation before continuation.
2. Remove any source directories left empty by the deletion and verify the
   target directory is gone or non-empty for an intentional reason.
3. Update `.codex-qemu-kernels/state.json`.
4. Amend or create the source commit as appropriate, committing kernel source
   changes only. Preserve the coarse coherent deletion-unit rule and keep the
   commit message clear.
5. Record the review under `.codex-qemu-kernels/reviews/`.
6. Return to normal continuation if clean.
