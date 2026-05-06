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

work_dir=".codex-qemu-kernels"
src="$work_dir/init.c"

case "$arch" in
x86_64)
	cc="${CC_X86_64:-gcc}"
	rootfs="$work_dir/rootfs-x86_64"
	init_bin="$work_dir/init-x86_64"
	cpio="$work_dir/initramfs-x86_64.cpio"
	;;
arm64)
	cc="${CC_ARM64:-aarch64-linux-gnu-gcc}"
	rootfs="$work_dir/rootfs-arm64"
	init_bin="$work_dir/init-arm64"
	cpio="$work_dir/initramfs-arm64.cpio"
	;;
*)
	usage
	;;
esac

command -v "$cc" >/dev/null 2>&1 || {
	echo "missing compiler: $cc" >&2
	exit 2
}

echo "==> build initramfs: $arch"
rm -rf "$rootfs"
mkdir -p "$rootfs"
"$cc" -static -O2 -Wall -Wextra -o "$init_bin" "$src"
cp "$init_bin" "$rootfs/init"
chmod 0755 "$rootfs/init"

(
	cd "$rootfs"
	find . -print | cpio -o -H newc
) >"$cpio"

printf '%s\n' "$cpio"
