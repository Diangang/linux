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
log_dir="$work_dir/logs/qemu"
mkdir -p "$log_dir"

timeout_s="${QEMU_TIMEOUT:-45}"
attempts="${QEMU_ATTEMPTS:-2}"
stamp="$(date +%Y%m%d-%H%M%S)"
log_file="$log_dir/minix-storage-$arch-$stamp.log"

"$work_dir/scripts/prepare-qemu-storage-images.sh"
"$work_dir/scripts/build-initramfs.sh" "$arch"

case "$arch" in
x86_64)
	kernel="$work_dir/build-x86_64/arch/x86/boot/bzImage"
	initramfs="$work_dir/initramfs-x86_64.cpio"
	console="ttyS0"
	qemu_cmd="qemu-system-x86_64"
	qemu_args="-M pc -cpu max"
	;;
arm64)
	kernel="$work_dir/build-arm64/arch/arm64/boot/Image"
	initramfs="$work_dir/initramfs-arm64.cpio"
	console="ttyAMA0"
	qemu_cmd="qemu-system-aarch64"
	qemu_args="-M virt -cpu cortex-a57"
	;;
*)
	usage
	;;
esac

if [ ! -s "$kernel" ]; then
	echo "missing kernel image: $kernel" >&2
	exit 2
fi
if [ ! -s "$initramfs" ]; then
	echo "missing initramfs image: $initramfs" >&2
	exit 2
fi

attempt=1
while [ "$attempt" -le "$attempts" ]; do
	if [ "$attempts" -gt 1 ]; then
		log_file="$log_dir/minix-storage-$arch-$stamp-attempt$attempt.log"
	fi
	qemu_err="$log_dir/minix-storage-$arch-$stamp-attempt$attempt.qemu.err"
	rm -f "$log_file" "$qemu_err"
	echo "==> run QEMU Minix storage test: $arch attempt=$attempt/$attempts timeout=${timeout_s}s log=$log_file"
	echo "==> qemu command: $qemu_cmd $qemu_args -m 1024M -smp 2 -display none -serial file:$log_file -monitor none -no-user-config -nodefaults -no-reboot -kernel $kernel -initrd $initramfs -drive file=$work_dir/nvme-test.raw,if=none,id=nvme0,format=raw -device nvme,drive=nvme0,serial=codexnvme0 -device virtio-scsi-pci,id=scsi0 -drive file=$work_dir/hdd-test.raw,if=none,id=hdd0,format=raw -device scsi-hd,drive=hdd0,bus=scsi0.0 -append console=$console rdinit=/init panic=-1"
	set +e
	# shellcheck disable=SC2086
	timeout "$timeout_s" "$qemu_cmd" \
		$qemu_args \
		-m 1024M \
		-smp 2 \
		-display none \
		-serial "file:$log_file" \
		-monitor none \
		-no-user-config \
		-nodefaults \
		-no-reboot \
		-kernel "$kernel" \
		-initrd "$initramfs" \
		-drive "file=$work_dir/nvme-test.raw,if=none,id=nvme0,format=raw" \
		-device nvme,drive=nvme0,serial=codexnvme0 \
		-device virtio-scsi-pci,id=scsi0 \
		-drive "file=$work_dir/hdd-test.raw,if=none,id=hdd0,format=raw" \
		-device scsi-hd,drive=hdd0,bus=scsi0.0 \
		-append "console=$console rdinit=/init panic=-1" \
		>"$qemu_err" 2>&1
	qemu_status=$?
	set -e
	if [ -s "$qemu_err" ]; then
		echo "==> qemu stderr: $qemu_err"
		cat "$qemu_err"
	fi
	if [ -f "$log_file" ]; then
		cat "$log_file"
	fi

	if [ -f "$log_file" ] && grep -q "CODEX_MINIX_TEST_PASS" "$log_file"; then
		break
	fi
	echo "missing pass marker in $log_file" >&2
	if [ "$qemu_status" -eq 124 ]; then
		echo "qemu timed out after ${timeout_s}s" >&2
	elif [ "$qemu_status" -ne 0 ]; then
		echo "qemu exited with status $qemu_status" >&2
	fi
	if [ "$attempt" -eq "$attempts" ]; then
		exit 1
	fi
	attempt=$((attempt + 1))
	sleep 1
done

echo "==> QEMU Minix storage test passed: $arch"
printf '%s\n' "$log_file"
