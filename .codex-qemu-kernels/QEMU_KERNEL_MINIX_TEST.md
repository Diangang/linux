# QEMU initramfs Minix storage test

This note records the fixed validation contract for the long-running kernel
trimming task.

The test builds x86_64 and arm64 kernels, boots each one under QEMU with a
generated initramfs, attaches one NVMe disk and one HDD-like SCSI disk, mounts
both as Minix, writes and reads test data, syncs, unmounts, and powers off.

The validation environment intentionally does not use a Debian root filesystem,
`root=/dev/vda1`, `init=/bin/bash`, or virtio-blk as a root-device dependency.
This keeps `VIRTIO_BLK` outside the trimming keep list.

## Workspace

All generated files stay under:

```sh
.codex-qemu-kernels/
```

This directory is a local build/test/control workspace and is not committed.

Important files:

```sh
.codex-qemu-kernels/init.c
.codex-qemu-kernels/scripts/build-kernel.sh
.codex-qemu-kernels/scripts/build-initramfs.sh
.codex-qemu-kernels/scripts/prepare-qemu-storage-images.sh
.codex-qemu-kernels/scripts/run-minix-storage-test.sh
.codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh
```

Generated images:

```sh
.codex-qemu-kernels/build-x86_64/arch/x86/boot/bzImage
.codex-qemu-kernels/build-arm64/arch/arm64/boot/Image
.codex-qemu-kernels/initramfs-x86_64.cpio
.codex-qemu-kernels/initramfs-arm64.cpio
.codex-qemu-kernels/nvme-test.raw
.codex-qemu-kernels/hdd-test.raw
```

## Dependencies

Required host tools:

```sh
apt-get update
apt-get install -y \
  qemu-system-x86 qemu-system-arm qemu-utils \
  gcc-aarch64-linux-gnu \
  flex bison bc libssl-dev libelf-dev dwarves \
  util-linux cpio
```

The host-side setup uses `mkfs.minix -3` from util-linux to format the
disposable raw test disks before QEMU boots.

## Build Kernels

Build both architectures:

```sh
.codex-qemu-kernels/scripts/build-kernel.sh x86_64
.codex-qemu-kernels/scripts/build-kernel.sh arm64
```

The build script defaults to `JOBS=32` and `CLEAN_BUILD=1`, so each validation
build removes the architecture-specific `O=` directory before rebuilding.
Override either value when needed:

```sh
JOBS=16 .codex-qemu-kernels/scripts/build-kernel.sh x86_64
CLEAN_BUILD=0 .codex-qemu-kernels/scripts/build-kernel.sh x86_64
```

The build script enables the fixed storage-test dependencies:

- `CONFIG_BLK_DEV_NVME`
- `CONFIG_NVME_CORE`
- `CONFIG_MINIX_FS`
- `CONFIG_SCSI`
- `CONFIG_BLK_DEV_SD`
- `CONFIG_SCSI_VIRTIO`
- `CONFIG_VIRTIO_PCI`
- `CONFIG_BLK_DEV_INITRD`
- `CONFIG_DEVTMPFS`
- `CONFIG_DEVTMPFS_MOUNT`

It does not enable `CONFIG_VIRTIO_BLK` for the validation root path.

## Build Initramfs

Build both initramfs images:

```sh
.codex-qemu-kernels/scripts/build-initramfs.sh x86_64
.codex-qemu-kernels/scripts/build-initramfs.sh arm64
```

The initramfs contains a static `/init` built from
`.codex-qemu-kernels/init.c`. The init process mounts devtmpfs, waits for
`/dev/nvme0n1` and `/dev/sda`, mounts both as Minix, writes and reads 4 MiB on
each filesystem, syncs, unmounts, prints `CODEX_MINIX_TEST_PASS`, and powers
off.

## Prepare Disks

Prepare disposable raw test disks:

```sh
.codex-qemu-kernels/scripts/prepare-qemu-storage-images.sh
```

This recreates and formats:

```sh
.codex-qemu-kernels/nvme-test.raw
.codex-qemu-kernels/hdd-test.raw
```

Both are formatted with:

```sh
mkfs.minix -3
```

## Run QEMU Tests

Run per architecture:

```sh
.codex-qemu-kernels/scripts/run-minix-storage-test.sh x86_64
.codex-qemu-kernels/scripts/run-minix-storage-test.sh arm64
```

The QEMU script defaults to `QEMU_TIMEOUT=45` and `QEMU_ATTEMPTS=2`. This keeps
empty-output QEMU launch failures from stalling validation for several minutes.

The x86_64 QEMU shape is:

```sh
qemu-system-x86_64 \
  -M pc \
  -cpu max \
  -m 1024M \
  -smp 2 \
  -display none \
  -serial file:.codex-qemu-kernels/logs/qemu/minix-storage-x86_64.log \
  -monitor none \
  -no-user-config \
  -nodefaults \
  -no-reboot \
  -kernel .codex-qemu-kernels/build-x86_64/arch/x86/boot/bzImage \
  -initrd .codex-qemu-kernels/initramfs-x86_64.cpio \
  -drive file=.codex-qemu-kernels/nvme-test.raw,if=none,id=nvme0,format=raw \
  -device nvme,drive=nvme0,serial=codexnvme0 \
  -device virtio-scsi-pci,id=scsi0 \
  -drive file=.codex-qemu-kernels/hdd-test.raw,if=none,id=hdd0,format=raw \
  -device scsi-hd,drive=hdd0,bus=scsi0.0 \
  -append "console=ttyS0 rdinit=/init panic=-1"
```

The arm64 QEMU shape is the same storage contract with:

```sh
qemu-system-aarch64 -M virt -cpu cortex-a57
```

and:

```sh
-kernel .codex-qemu-kernels/build-arm64/arch/arm64/boot/Image
-initrd .codex-qemu-kernels/initramfs-arm64.cpio
-append "console=ttyAMA0 rdinit=/init panic=-1"
```

## Full Contract

Run the whole validation contract:

```sh
.codex-qemu-kernels/scripts/validate-fixed-storage-contract.sh
```

Success requires both architecture logs to contain:

```text
CODEX_MINIX_TEST_PASS
```

## Trimming Implication

The validation keep list includes NVMe, SCSI core, SCSI disk, virtio-scsi,
Minix, initramfs, devtmpfs, and the QEMU PCI/interrupt path needed by those
devices.

Other filesystems and other block device drivers remain trimming candidates.
