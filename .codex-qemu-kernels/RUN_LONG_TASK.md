# Running the Linux minimal core long task

The long task is controlled entirely from `.codex-qemu-kernels/`.

The kernel source tree is the trimming target. Source commits should contain
only kernel source deletions, build/config reference cleanup, or
content-equivalent moves. Do not commit `.codex-qemu-kernels/`.

One source commit removes one complete module, subsystem slice, or feature
family. Commit messages must clearly name what was removed, why it is outside
the fixed validation contract, and that x86_64 plus arm64 validation passed.

Only x86_64 and arm64 are required validation targets for this task.

## Multi-agent operating pattern (non-persistent sessions)

Sub-agent IDs (`Hilbert` for validation and `Carver` for modifications) are
session-local. After a process restart, only Main agent continuity is guaranteed
through `.codex-qemu-kernels/state.json`; agent IDs may need to be recreated.

Workflow each round:

1. Main agent performs the round analysis and identifies the exact module,
   directory scope, and file list to touch.
2. Main agent sends a narrow, explicit modification ticket to the modify-only
   agent (`Carver` in this session model).
3. Main agent reviews returned patch scope and only then dispatches the selected
   validation command to the validation-only agent (`Hilbert` in this session
   model).

Never let a modify pass start before the round scope is fixed. Never let a test
pass decide code edits; validation only confirms the already approved round scope.

## Dry run

From the repository root:

```sh
CODEX_DRY_RUN=1 .codex-qemu-kernels/scripts/codex-supervisor.sh
```

## Run one supervised round

```sh
CODEX_MAX_ROUNDS=1 .codex-qemu-kernels/scripts/codex-supervisor.sh
```

## Run continuously

```sh
.codex-qemu-kernels/scripts/codex-supervisor.sh
```

## Useful environment overrides

```sh
CODEX_BIN=codex
CODEX_ARGS=exec
CODEX_MAX_ROUNDS=1
CODEX_ROUND_TIMEOUT=0
CODEX_STREAM_LOG=1
CODEX_LOG_DIR=.codex-qemu-kernels/logs/agent-runs
JOBS=32
CLEAN_BUILD=1
QEMU_TIMEOUT=45
QEMU_ATTEMPTS=2
```

## Current resume point

Current source HEAD is `9423d475c3c4`. The compact current analysis, progress,
and next plan live in `.codex-qemu-kernels/CURRENT_PROJECT_STATE.md`.

The automated validation boots with a generated initramfs, not a Debian qcow2
root filesystem. Do not make `VIRTIO_BLK` part of the validation contract.

Required validation:

- build x86_64;
- build arm64;
- boot x86_64 with NVMe and HDD-like SCSI disks;
- boot arm64 with NVMe and HDD-like SCSI disks;
- recreate and format both disposable raw disk images with `mkfs.minix -3`;
- mount both as Minix;
- write/read/sync/unmount both filesystems.

Other architectures are out of scope unless a changed shared file directly
affects x86_64 or arm64.

## Stop and review cases

Stop for human review if:

- existing runtime logic must change;
- a stub seems necessary;
- the fixed NVMe or SCSI HDD Minix test fails;
- a dependency is unclear;
- a source deletion would touch core VFS, block, NVMe, SCSI, Minix, boot,
  syscall/UAPI, locking, lifetime, or I/O behavior in a non-mechanical way.
