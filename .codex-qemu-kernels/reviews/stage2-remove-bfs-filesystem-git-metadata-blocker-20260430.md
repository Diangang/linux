# Stage 2 BFS Removal Bugfix Review

## Target

Recover the validated BFS filesystem deletion patch without changing runtime
logic.

## Bugfix scope check

No source edit was made in this bugfix pass. The tree still contains only the
validated BFS deletion patch:

- removed `fs/bfs/` implementation files;
- removed `fs/Kconfig` and `fs/Makefile` BFS wiring;
- removed stale `CONFIG_BFS_FS` selections from affected defconfigs;
- removed the stale `MAINTAINERS` `F: fs/bfs/` pattern.

Checked for stale mechanical fallout:

```sh
rg -n "CONFIG_BFS_FS|source \"fs/bfs/Kconfig\"|obj-\$\(CONFIG_BFS_FS\)|F:\s*fs/bfs/" MAINTAINERS fs arch/mips/configs arch/powerpc/configs
git diff --check
```

Result: no stale BFS build/config/maintainer references in the edited source
scope, and `git diff --check` passed.

## Validation

No validation was rerun because no source change was made during this bugfix
pass. The current BFS deletion patch remains covered by the previous fixed
initramfs-based validation:

```sh
JOBS=8 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh
```

Recorded passing logs:

- `.codex-qemu-kernels/logs/qemu/minix-storage-x86_64-20260430-235426-attempt1.log`
- `.codex-qemu-kernels/logs/qemu/minix-storage-arm64-20260430-235429-attempt1.log`

## Blocker

Commit and post-commit review cannot proceed because repository metadata is
still mounted read-only:

```text
/data25/lidg/diangang/.git ext4 ro
```

`test -w .git` reports `.git not writable`. Creating the source commit requires
`.git/index.lock`, so this cannot be completed from the current sandbox state.

## Recovery

Make `/data25/lidg/diangang/.git` writable, then stage and commit only the
kernel source files for `stage2-remove-bfs-filesystem`. Do not commit
`.codex-qemu-kernels/`.
