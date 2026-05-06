# Stage 2 BFS Removal Blocked Review

## Target

Remove the SCO UnixWare BFS filesystem implementation and its build/config
wiring.

## Patch class

delete_plus_build_wiring

## Validation

Passed:

```sh
JOBS=8 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh
```

Observed fixed contract pass:

- x86_64 build and generated-initramfs QEMU Minix NVMe/SCSI storage test passed.
- arm64 build and generated-initramfs QEMU Minix NVMe/SCSI storage test passed.
- QEMU logs:
  - `.codex-qemu-kernels/logs/qemu/minix-storage-x86_64-20260430-235426-attempt1.log`
  - `.codex-qemu-kernels/logs/qemu/minix-storage-arm64-20260430-235429-attempt1.log`

## Commit status

Blocked before commit. `git add` failed because the repository metadata mount is
read-only:

```text
fatal: Unable to create '/data25/lidg/diangang/.git/index.lock': Read-only file system
```

`mount` shows the worktree path mounted read-write and `.git` mounted read-only:

```text
/dev/nvme0n1p3 on /data25/lidg/diangang type ext4 (rw,nosuid,nodev,noatime,discard,nobarrier,stripe=32)
/dev/nvme0n1p3 on /data25/lidg/diangang/.git type ext4 (ro,nosuid,nodev,noatime,discard,nobarrier,stripe=32)
```

## Findings

No source review findings were reached because no commit could be created for
`git show --stat`, `git show --check`, or full commit diff review.

## Recovery

The validated source patch remains in the worktree. Remount or otherwise make
`.git` writable, then stage only kernel source files and commit the BFS removal.
