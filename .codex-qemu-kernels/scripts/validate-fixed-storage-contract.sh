#!/bin/sh
set -eu

repo_root="$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)"
cd "$repo_root"

scripts=".codex-qemu-kernels/scripts"

echo "==> validate fixed storage contract"
"$scripts/build-kernel.sh" x86_64
"$scripts/build-kernel.sh" arm64
"$scripts/run-minix-storage-test.sh" x86_64
"$scripts/run-minix-storage-test.sh" arm64
"$scripts/collect-metrics.sh"
echo "==> fixed storage contract passed"
