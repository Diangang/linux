# Deploying the QEMU kernel control workspace

This directory contains the recoverable task state, prompts, scripts, fixed
validation configs, reviews, metrics, and QEMU validation logs needed to resume
the trimming workflow on another machine.

Generated artifacts are intentionally not committed:

- `.codex-qemu-kernels/build-*`
- `.codex-qemu-kernels/*.raw`
- `.codex-qemu-kernels/init-*`
- `.codex-qemu-kernels/initramfs-*.cpio`
- `.codex-qemu-kernels/rootfs-*`
- `.codex-qemu-kernels/logs`

After cloning, regenerate validation artifacts from the repository root:

```sh
.codex-qemu-kernels/scripts/prepare-qemu-storage-images.sh
.codex-qemu-kernels/scripts/build-initramfs.sh x86_64
.codex-qemu-kernels/scripts/build-initramfs.sh arm64
JOBS=32 .codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh
```

Resume instructions live in `.codex-qemu-kernels/state.json`.
