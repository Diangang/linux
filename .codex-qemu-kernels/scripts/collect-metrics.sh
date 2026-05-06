#!/bin/sh
set -eu

repo_root="$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)"
cd "$repo_root"

out_dir=".codex-qemu-kernels/metrics"
mkdir -p "$out_dir"
out="$out_dir/metrics-$(date +%Y%m%d-%H%M%S).txt"

{
	echo "date_utc=$(date -u +%Y-%m-%dT%H:%M:%SZ)"
	for item in \
		.codex-qemu-kernels/build-x86_64/arch/x86/boot/bzImage \
		.codex-qemu-kernels/build-arm64/arch/arm64/boot/Image
	do
		if [ -f "$item" ]; then
			printf 'image_size_bytes %s ' "$item"
			wc -c <"$item"
		else
			echo "image_missing $item"
		fi
	done
	for cfg in \
		.codex-qemu-kernels/build-x86_64/.config \
		.codex-qemu-kernels/build-arm64/.config
	do
		if [ -f "$cfg" ]; then
			printf 'enabled_config_count %s ' "$cfg"
			grep -c '^CONFIG_.*=y' "$cfg" || true
			printf 'module_config_count %s ' "$cfg"
			grep -c '^CONFIG_.*=m' "$cfg" || true
		else
			echo "config_missing $cfg"
		fi
	done
} >"$out"

printf '%s\n' "$out"
