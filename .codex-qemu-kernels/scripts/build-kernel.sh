#!/bin/sh
set -eu

usage() {
	echo "usage: $0 x86_64|arm64" >&2
	exit 2
}

[ "$#" -eq 1 ] || usage
arch="$1"

repo_root="$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)"
cd "$repo_root"

jobs="${JOBS:-32}"
clean_build="${CLEAN_BUILD:-1}"

case "$arch" in
x86_64)
	build_dir=".codex-qemu-kernels/build-x86_64"
	image="$build_dir/arch/x86/boot/bzImage"
	echo "==> build kernel: x86_64 jobs=$jobs clean=$clean_build"
	if [ "$clean_build" = "1" ]; then
		rm -rf "$build_dir"
	fi
	make O="$build_dir" x86_64_defconfig
	./scripts/config --file "$build_dir/.config" \
		-e BLK_DEV_NVME \
		-e NVME_CORE \
		-e MINIX_FS \
		-e SCSI \
		-e BLK_DEV_SD \
		-e SCSI_VIRTIO \
		-e VIRTIO_PCI \
		-e BLK_DEV_INITRD \
		-e DEVTMPFS \
		-e DEVTMPFS_MOUNT
	make O="$build_dir" olddefconfig
	make -j"$jobs" O="$build_dir"
	;;
arm64)
	build_dir=".codex-qemu-kernels/build-arm64"
	image="$build_dir/arch/arm64/boot/Image"
	echo "==> build kernel: arm64 jobs=$jobs clean=$clean_build"
	if [ "$clean_build" = "1" ]; then
		rm -rf "$build_dir"
	fi
	make O="$build_dir" ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- defconfig
	./scripts/config --file "$build_dir/.config" \
		-e BLK_DEV_NVME \
		-e NVME_CORE \
		-e MINIX_FS \
		-e SCSI \
		-e BLK_DEV_SD \
		-e SCSI_VIRTIO \
		-e VIRTIO_PCI \
		-e BLK_DEV_INITRD \
		-e DEVTMPFS \
		-e DEVTMPFS_MOUNT
	make O="$build_dir" ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- olddefconfig
	make -j"$jobs" O="$build_dir" ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu-
	;;
*)
	usage
	;;
esac

test -s "$image"
printf '%s\n' "$image"
