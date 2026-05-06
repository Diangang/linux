#!/bin/sh
set -eu

repo_root="$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)"
cd "$repo_root"

work_dir=".codex-qemu-kernels"
mkdir -p "$work_dir"

create_disk() {
	out="$1"

	echo "==> prepare Minix disk: $out"
	rm -f "$out"
	qemu-img create -f raw "$out" 256M
	mkfs.minix -3 "$out" >/dev/null
}

command -v qemu-img >/dev/null 2>&1 || {
	echo "missing qemu-img" >&2
	exit 2
}
command -v mkfs.minix >/dev/null 2>&1 || {
	echo "missing mkfs.minix" >&2
	exit 2
}

create_disk "$work_dir/nvme-test.raw"
create_disk "$work_dir/hdd-test.raw"
