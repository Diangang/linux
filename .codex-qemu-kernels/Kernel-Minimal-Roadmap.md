# Linux minimal core trimming roadmap

This roadmap defines the long-running task for trimming this Linux source tree
down to the smallest kernel that still satisfies the fixed QEMU storage
contract.

All task control files, prompts, logs, generated configs, QEMU images,
initramfs images, and review notes live under `.codex-qemu-kernels/` and are not
part of source commits. Commits are for kernel source changes only.

The supported architecture scope for this trimming task is exactly x86_64 and
arm64. Other architectures are out of scope and should not block progress unless
their files are directly shared with x86_64 or arm64.

The validation environment boots a generated initramfs with `rdinit=/init`.
It must not use a Debian root filesystem, `root=/dev/vda1`, or virtio-blk as a
root device dependency. The disposable NVMe and SCSI HDD images are recreated
and formatted as Minix by the host-side validation setup before each QEMU run;
the initramfs then mounts, writes, reads, syncs, and unmounts both devices.

## Hard validation contract

Every accepted source commit must pass all required validation:

1. Build x86_64 kernel image.
2. Build arm64 kernel image.
3. Boot x86_64 under QEMU.
4. Boot arm64 under QEMU.
5. Attach one NVMe disk and one HDD-like SCSI disk.
6. Format both disks as Minix.
7. Mount both Minix filesystems.
8. Write, read, sync, and unmount both filesystems.

Validation of other architectures is not required for this task.

The required devices are fixed:

- NVMe: `/dev/nvme0n1`
- HDD-like SCSI disk: `/dev/sda`

The required filesystem is fixed:

- Minix, created with `mkfs.minix -3`

Other filesystems and other block device drivers are default trimming
candidates unless a dependency ledger proves they are required by the fixed
validation contract.

## Source modification policy

This task is deletion-first. Kernel source commits may contain only:

- deleting source files or directories;
- deleting Kconfig entries, Makefile targets, includes, declarations, or other
  now-dead build/config references;
- moving code only when the moved content is equivalent.

Do not change existing runtime logic to make trimming pass.

If a patch requires changing existing function bodies, control flow, data
structure semantics, error handling, locking, reference counting, I/O behavior,
mount/read/write behavior, syscall/UAPI behavior, device names, or replacing
removed code with stubs, set `run_control.stop_condition` to
`logic_change_required` and stop for human review.

Mechanical build fallout from deletion may be fixed automatically only when it
removes dead references and does not alter runtime behavior.

## Trimming order

The long task must prioritize coarse deletion before fine-grained cleanup.

1. First remove whole feature directories or complete subsystem directories
   that are outside the fixed validation contract.
2. After no safe directory-level target remains in the current broad area,
   remove whole source files that become unused or unbuilt.
3. Only after directory-level and whole-file deletion opportunities are
   exhausted should the task enter file-internal trimming such as deleting
   individual functions, data tables, cases, declarations, or struct fields.

Do not start file-internal minimization while a safe whole directory or whole
file deletion target remains. File-internal work belongs to the later core
surface reduction phase unless it is strictly required as mechanical fallout
from a directory or file deletion.

## Temporary compatibility moves

`lib/ucs2_string.c` currently carries UTF-8/UTF-16 conversion helpers moved out
of the deleted `fs/nls/` implementation. This is a temporary build-compatibility
move for remaining non-`fs/nls` users such as USB, WMI, ACPI, or other firmware
and device paths that still reference `utf8_to_utf32`, `utf32_to_utf8`,
`utf8s_to_utf16s`, or `utf16s_to_utf8s`.

When later trimming removes all remaining callers of those exported UTF helper
symbols, delete the moved helper block from `lib/ucs2_string.c` as dead code.

### Temporary compatibility ledger

2026-05-01 root directory deletion pass:

- `kernel/capability.c`: temporary core-side replacements for capability,
  mmap-min-address, xattr, ptrace, and VM hooks that used to come from
  `security/commoncap.c` and `security/min_addr.c`. Remove these additions when
  the remaining syscall, xattr, VFS, and mm callers are deleted or no longer
  require those exported symbols.
- `kernel/sched/core.c`: removed the direct `io_uring/io-wq.h` include and
  io-wq worker sleep/run callbacks after deleting `io_uring/`. No restore is
  needed unless io_uring support returns.
- `init/Kconfig`: `SYSVIPC`, `CGROUP_DEVICE`, and `CGROUP_BPF` are temporarily
  forced behind `BROKEN` because `ipc/`, `security/device_cgroup.c`, and net/BPF
  support were deleted. Delete the remaining Kconfig entries when their callers
  are removed.
- `kernel/power/Kconfig`: `HIBERNATION` is temporarily forced behind `BROKEN`
  and its crypto selects were removed because `crypto/` was deleted. Delete the
  hibernation code in a later power-management trimming pass if it remains
  outside the validation contract.
- `fs/Kconfig`: removed the `IO_WQ` symbol because its only kept purpose was
  io_uring support. Drop any remaining IO_WQ references when found.
- `Kbuild`, root `Kconfig`, `drivers/Kconfig`, and `lib/Kconfig.debug`: deleted
  build and Kconfig wiring for the removed root directories. These removals are
  expected to be permanent for the minimal fixed-storage target.

2026-05-02 arch directory deletion pass:

- Top-level `Makefile`: removed the default `all: dtbs` edge for
  `CONFIG_OF_EARLY_FLATTREE` because deleting 32-bit `arch/arm/` leaves arm64
  DTS include targets unavailable. Explicit `dtbs` and builtin-DTB dependencies
  remain available; the fixed storage validation builds kernel images, not full
  board DTB inventories.
- `arch/arm64/configs/defconfig`: disabled Xen because arm64 Xen still reused
  deleted `arch/arm/xen/` objects. This can stay disabled for the fixed QEMU
  storage target unless Xen support becomes a required validation dependency.
- `arch/x86/Kbuild`, `arch/x86/Makefile`, `arch/x86/platform/Makefile`,
  `arch/x86/pci/Makefile`, `arch/x86/kernel/apic/Makefile`,
  `arch/x86/Kconfig`, `arch/x86/Kconfig.debug`,
  `arch/x86/Kconfig.cpufeatures`, `arch/x86/kernel/head_64.S`,
  `arch/arm64/Kbuild`, `arch/arm64/Kconfig`, and
  `arch/arm64/configs/defconfig`: removed stale build/config/include wiring for
  deleted x86/arm64 subdirectories. These are expected permanent for the fixed
  storage target but are recorded for later rollback review.

2026-05-02 drivers directory deletion pass:

- `arch/x86/kernel/rtc.c` and `arch/x86/kernel/x86_init.c`: x86 persistent
  wallclock hooks now use the existing no-op RTC accessors after deleting
  `drivers/rtc/` and disabling x86 RTC library selection. Revisit and delete
  any remaining RTC wallclock compatibility surface when the later timekeeping
  and platform-device passes prove it is unused by the fixed QEMU storage
  target.
- `drivers/Makefile` and `drivers/Kconfig`: removed build and Kconfig wiring
  for deleted driver directories. These are expected permanent deletions for
  the fixed-storage target, but are recorded here because they were added as
  mechanical fallout rather than pure `rm -rf`.
- `drivers/acpi/Kconfig`, `drivers/clk/hisilicon/Kconfig`,
  `drivers/clk/qcom/Kconfig`, `drivers/gpio/Kconfig`,
  `drivers/iommu/Kconfig`, `drivers/pci/Kconfig`, `drivers/tty/Kconfig`,
  `drivers/tty/serial/8250/Kconfig`, `kernel/power/Kconfig`, and
  `lib/Kconfig.debug`: changed removed-subsystem `select` edges into
  dependencies or removed stale sources so deleted driver subsystems are not
  forced back on by defconfig. Revisit these after the kept driver directories
  are trimmed internally; many entries should disappear with their containing
  drivers.
- `drivers/block/Kconfig`, `drivers/block/Makefile`,
  `drivers/char/Kconfig`, `drivers/char/Makefile`,
  `drivers/nvme/Kconfig`, `drivers/nvme/Makefile`,
  `drivers/nvme/common/Kconfig`, `drivers/nvme/common/Makefile`,
  `drivers/of/Kconfig`, `drivers/of/Makefile`, `drivers/pnp/Kconfig`,
  `drivers/pnp/Makefile`, `arch/arm64/configs/defconfig`, and
  `kernel/configs/xen.config`: removed stale build/config wiring for the
  deleted Stage 3 subdirectories. These are expected permanent for the fixed
  storage target but are recorded for later rollback review.

## Commit granularity

Current Stage 6 source commit queue as of 2026-05-05:

- Done: `1bda875da9de` removed fixed-target debugfs support by disabling arm64
  `DEBUG_FS`/`BLK_DEBUG_FS`, deleting `fs/debugfs/`, and validating the fixed
  x86_64 plus arm64 QEMU Minix storage contract.
- Done: `eb39b20a24b9` removed the no-fixed-object `drivers/gpio/Kconfig`
  surface and `drivers/idle/` files, removed the physical empty directories
  from the worktree, and validated the fixed x86_64 plus arm64 QEMU Minix
  storage contract.
- Done: `0527da1f93e8` removed arm64 non-storage module residue for PCI
  pwrctrl, Google firmware, Xilinx/DW DMA, and Xilinx clock support. The fixed
  arm64 config now leaves only `PSTORE_RAM` and `REED_SOLOMON` as modules.
- Next: reassess Stage 6 for the next coarse whole-directory or whole-file
  deletion target.
- Keep: `fs/pstore/` stays for panic persistence/debuggability.

Each source commit should prefer one coarse, coherent deletion unit: a complete
directory, a top-level subsystem slice, or a set of whole files that form one
obvious feature family. Do not split a directory-level deletion into many
file-internal edits. Do not start line-by-line minimization when deleting the
whole file or whole directory is safe.

Bundling is acceptable when the deleted directories/files are tightly coupled
and must be removed together to keep the tree buildable, but avoid combining
independent areas that would obscure validation failures. A commit is too small
if it leaves a half-removed feature with obvious dead build/config surface; it
is too fine-grained if it edits inside files before directory and whole-file
targets have been exhausted.

Each source commit must have a clear commit message naming:

- the module or feature being removed;
- why it is outside the fixed validation contract;
- the x86_64 and arm64 validation that passed.

Examples of acceptable commit scopes:

- remove one or more tightly coupled filesystem directories and their
  Kconfig/Makefile entries;
- remove an unused block driver directory family and its build/config entries;
- remove a non-storage subsystem directory tree that is independent from
  validation;
- remove a group of whole source files that belong to one feature family.

Examples of unacceptable commit scopes:

- remove storage and networking in the same commit;
- make runtime logic changes while deleting a feature;
- leave a removed feature half wired in Kconfig or Makefiles.
- delete individual functions inside files while whole directories or whole
  files remain safe deletion targets.

## Initial keep list

The keep list is intentionally narrow and may be refined only when validation
or dependency analysis proves a missing item is required.

Storage and filesystem:

- `fs/minix/`
- VFS core required for mount, open, read, write, sync, and unmount
- block core required for bio/request/blk-mq/gendisk/partition handling
- `drivers/nvme/host/`
- SCSI core required by a QEMU HDD-like disk
- SCSI disk support

Boot and runtime:

- x86_64 and arm64 boot paths
- initramfs support
- devtmpfs or equivalent device-node path required by validation
- proc/sysfs pieces required by validation scripts
- serial console and panic/poweroff paths
- PCI and interrupt paths required by QEMU NVMe/SCSI

Everything else starts as a trimming candidate.

## Stages

### Stage 0: Control Plane

Status: active

Create and maintain the `.codex-qemu-kernels/` long-task workspace:

- `state.json`
- supervisor script
- normal and bugfix prompts
- roadmap
- review notes
- logs
- validation scripts

No kernel source commit is required for this stage.

### Stage 1: Automated Fixed Validation

Turn `.codex-qemu-kernels/QEMU_KERNEL_MINIX_TEST.md` into repeatable scripts.
Validation must be non-interactive enough for Codex/supervisor rounds.

Required outputs:

- build script for x86_64
- build script for arm64
- initramfs build script for x86_64 and arm64
- QEMU Minix storage test for x86_64
- QEMU Minix storage test for arm64
- metrics collection for image size, config counts, and boot/test result

### Stage 2: `fs/` Directory Pass

Current real `fs/` directories are:

- `fs/cramfs/`
- `fs/debugfs/`
- `fs/kernfs/`
- `fs/minix/`
- `fs/proc/`
- `fs/pstore/`
- `fs/ramfs/`
- `fs/sysfs/`

Process these as whole directories first. `fs/minix/` is required by the fixed
validation contract. `fs/pstore/` is intentionally kept for panic log
persistence and debugging value. `fs/proc/`, `fs/sysfs/`, `fs/kernfs/`, and
`fs/ramfs/` are likely boot/runtime support and need explicit dependency
ledgers before deletion. `fs/cramfs/` was removed in the 2026-05-02 whole-tree
coarse directory scan pass. The remaining first filesystem directory candidate
to ledger is `fs/debugfs/`.

### Stage 3: `drivers/` Directory Pass

Process actual `drivers/` subdirectories as whole-directory targets. Keep only
what the fixed x86_64/arm64 QEMU NVMe plus SCSI HDD validation path proves is
required.

Current real `drivers/` directories:

- `drivers/acpi/`, `drivers/amba/`, `drivers/base/`, `drivers/block/`,
  `drivers/bus/`, `drivers/char/`, `drivers/clk/`,
  `drivers/clocksource/`, `drivers/dma/`, `drivers/firmware/`,
  `drivers/gpio/`, `drivers/idle/`, `drivers/iommu/`, `drivers/irqchip/`,
  `drivers/nvme/`, `drivers/of/`, `drivers/pci/`, `drivers/pnp/`,
  `drivers/scsi/`, `drivers/tty/`, `drivers/virtio/`

Known keep/evaluate-late directories for the validation path include
`drivers/base/`, `drivers/block/`, `drivers/char/`, `drivers/firmware/`,
`drivers/nvme/`, `drivers/pci/`, `drivers/scsi/`, `drivers/tty/`,
`drivers/virtio/`, and the bus, clock, interrupt, ACPI/OF, and DMA paths
proven required by x86_64/arm64 QEMU boot. The broad non-validation driver
directory deletion pass is complete for now; next Stage 3 work should continue
with whole subdirectory or whole-file deletion inside the remaining kept
directories before any file-internal trimming.

Validated 2026-05-02 subdirectory pass removed:

- `drivers/block/aoe/`, `drivers/block/drbd/`,
  `drivers/block/mtip32xx/`, `drivers/block/null_blk/`,
  `drivers/block/rnbd/`, `drivers/block/rnull/`,
  `drivers/block/xen-blkback/`, and `drivers/block/zram/`
- `drivers/char/agp/`, `drivers/char/ipmi/`, `drivers/char/tpm/`,
  `drivers/char/xilinx_hwicap/`, and `drivers/char/xillybus/`
- `drivers/nvme/target/`, `drivers/nvme/common/tests/`,
  `drivers/of/unittest-data/`, `drivers/of/unittest.c`,
  `drivers/pnp/isapnp/`, and `drivers/pnp/pnpbios/`

Build/config fallout for that pass removed stale Kconfig and Makefile wiring
from the kept parent directories and dropped dead TPM/Xen config requests.

Validated 2026-05-02 whole-tree coarse directory scan removed additional
non-validation driver subdirectories:

- `drivers/base/test/`
- `drivers/bus/fsl-mc/` and `drivers/bus/mhi/`
- `drivers/acpi/dptf/`, `drivers/acpi/nfit/`, `drivers/acpi/pmic/`, and
  `drivers/acpi/riscv/`
- `drivers/pci/switch/`
- `drivers/iommu/iommufd/` and `drivers/iommu/riscv/`
- non-QEMU SCSI HBA, transport, and service directories:
  `drivers/scsi/aacraid/`, `drivers/scsi/aic7xxx/`,
  `drivers/scsi/aic94xx/`, `drivers/scsi/arcmsr/`,
  `drivers/scsi/arm/`, `drivers/scsi/be2iscsi/`,
  `drivers/scsi/bfa/`, `drivers/scsi/bnx2fc/`,
  `drivers/scsi/bnx2i/`, `drivers/scsi/csiostor/`,
  `drivers/scsi/cxgbi/`, `drivers/scsi/device_handler/`,
  `drivers/scsi/elx/`, `drivers/scsi/esas2r/`,
  `drivers/scsi/fcoe/`, `drivers/scsi/fnic/`,
  `drivers/scsi/hisi_sas/`, `drivers/scsi/ibmvscsi/`,
  `drivers/scsi/ibmvscsi_tgt/`, `drivers/scsi/isci/`,
  `drivers/scsi/libfc/`, `drivers/scsi/libsas/`,
  `drivers/scsi/lpfc/`, `drivers/scsi/megaraid/`,
  `drivers/scsi/mpi3mr/`, `drivers/scsi/mpt3sas/`,
  `drivers/scsi/mvsas/`, `drivers/scsi/pcmcia/`,
  `drivers/scsi/pm8001/`, `drivers/scsi/qedf/`,
  `drivers/scsi/qedi/`, `drivers/scsi/qla2xxx/`,
  `drivers/scsi/qla4xxx/`, `drivers/scsi/smartpqi/`,
  `drivers/scsi/snic/`, and `drivers/scsi/sym53c8xx_2/`

Build/config fallout for that pass removed stale Kconfig, Makefile, and arm64
defconfig wiring. It added no non-config source compatibility code.

### Stage 4: `arch/` Directory Pass

Current real `arch/` directories:

- `arch/arm64/`
- `arch/x86/`

Status: complete for the current architecture scope. `arch/x86/` and
`arch/arm64/` are the only kept architecture directories. Deleted non-target
architecture directories: `arch/alpha/`, `arch/arc/`, `arch/arm/`,
`arch/csky/`, `arch/hexagon/`, `arch/loongarch/`, `arch/m68k/`,
`arch/microblaze/`, `arch/mips/`, `arch/nios2/`, `arch/openrisc/`,
`arch/parisc/`, `arch/powerpc/`, `arch/riscv/`, `arch/s390/`, `arch/sh/`,
`arch/sparc/`, `arch/um/`, and `arch/xtensa/`.

Validated and committed 2026-05-02 x86/arm64 subdirectory pass
(`d0ea7e33d1bc`) removed:

- `arch/x86/kvm/`, `arch/x86/xen/`, `arch/x86/hyperv/`,
  `arch/x86/um/`, `arch/x86/coco/`, `arch/x86/math-emu/`,
  `arch/x86/video/`, `arch/x86/Makefile.um`
- `arch/x86/platform/atom/`, `arch/x86/platform/ce4100/`,
  `arch/x86/platform/geode/`, `arch/x86/platform/intel/`,
  `arch/x86/platform/intel-mid/`, `arch/x86/platform/intel-quark/`,
  `arch/x86/platform/iris/`, `arch/x86/platform/olpc/`,
  `arch/x86/platform/pvh/`, `arch/x86/platform/scx200/`,
  `arch/x86/platform/ts5500/`, and `arch/x86/platform/uv/`
- `arch/arm64/kvm/`, `arch/arm64/xen/`, `arch/arm64/hyperv/`,
  `arch/arm64/crypto/`, and `arch/arm64/boot/dts/`
- non-validation config fragments:
  `arch/x86/configs/hardening.config`, `arch/x86/configs/i386_defconfig`,
  `arch/x86/configs/tiny.config`, `arch/x86/configs/xen.config`,
  `arch/arm64/configs/hardening.config`, and `arch/arm64/configs/virt.config`

Build/config fallout for that pass removed stale Kbuild, Makefile, Kconfig,
defconfig, and x86 Xen head include references. Remaining arch directory
candidates should be config-backed: `arch/x86/events/`, `arch/x86/ia32/`,
`arch/x86/purgatory/`, `arch/x86/power/`, `arch/arm64/kernel/vdso32/`, and
`arch/arm64/net/` still have current config users and need a later config
reduction pass before deletion.

### Stage 5: Root Directory Pass

Current real top-level directories:

- `.codex-qemu-kernels/`
- `.git/`
- `arch/`
- `block/`
- `drivers/`
- `fs/`
- `include/`
- `init/`
- `kernel/`
- `lib/`
- `mm/`
- `scripts/`
- `tools/`
- `usr/`

Never treat `.git/` as a deletion target. `.codex-qemu-kernels/` is the control
workspace and is not committed. Keep or evaluate-late directories required for
building and booting the fixed contract: `arch/`, `block/`, `drivers/`, `fs/`,
`include/`, `init/`, `kernel/`, `lib/`, `mm/`, `scripts/`, `tools/`, and
`usr/`.

No root-level whole-directory deletion candidate is currently recorded as ready.
Previous root deletion passes already removed `certs/`, `crypto/`,
`Documentation/`, `io_uring/`, `ipc/`, `LICENSES/`, `net/`, `rust/`,
`samples/`, `security/`, `sound/`, and `virt/`.

Validated and committed 2026-05-02 `tools/` directory pass (`616246a3ce74`)
removed non-validation user-space tools, tests, perf/tracing/power/thermal
utilities, USB/virtio examples, verification helpers, non-x86 tools architecture
headers, `bpftool`, and unused tools build docs/tests. Kept `tools/objtool/`,
`tools/build/`, `tools/scripts/`, `tools/include/`, `tools/arch/x86/`,
`tools/lib/subcmd/`, `tools/lib/bpf/`, and `tools/bpf/resolve_btfids/` for
kernel host-tool builds. Mechanical fallout removed stale `tools/Makefile`,
`tools/bpf/Makefile`, top-level kselftest, perf source package, kernel-doc, and
bitmap runtime test wiring.

Validated 2026-05-02 `lib/`, `kernel/`, and `mm/` first coarse directory pass
removed debug/test/sanitizer/livepatch/liveupdate/DAMON/KUnit implementation
directories outside the fixed storage contract:

- `kernel/debug/`, `kernel/gcov/`, `kernel/kcsan/`, `kernel/livepatch/`, and
  `kernel/liveupdate/`
- `lib/kunit/`, `lib/tests/`, `lib/math/tests/`, `lib/crc/tests/`,
  `lib/crypto/tests/`, `lib/raid6/test/`, `lib/raid/xor/tests/`, and
  `lib/test_fortify/`
- `mm/damon/`, `mm/kasan/`, `mm/kfence/`, `mm/kmsan/`, and `mm/tests/`

Mechanical fallout removed stale Makefile and Kconfig wiring for those deleted
directories. The only non-config source addition was an explicit
`asm/debug-monitors.h` include in `arch/arm64/kernel/entry-common.c`, recorded
in the temporary compatibility ledger for later rollback review if arm64
hw-breakpoint/debug-monitor support is removed.

Validated 2026-05-02 `include/` header directory pass removed unused header
families outside the fixed storage contract:

- `include/math-emu/` and `include/ufs/`
- `include/linux/rtc/`
- vendor or accelerator helper header directories:
  `include/linux/avf/`, `include/linux/bnge/`, `include/linux/bnxt/`,
  `include/linux/pds/`, and `include/linux/qat/`
- network/filesystem service UAPI or helper headers:
  `include/linux/lockd/`, `include/uapi/linux/cifs/`, and
  `include/uapi/linux/nfsd/`

This pass was pure header-directory deletion. It added no non-config source
compatibility code; the temporary compatibility ledger records that explicitly
for later rollback review.

Validated 2026-05-02 whole-tree coarse directory scan removed `fs/cramfs/`,
root `virt/`, driver test, bus, ACPI, PCI switch, IOMMUFD/RISC-V IOMMU, and
non-QEMU SCSI HBA/service directories. This pass was deletion plus
Kconfig/Makefile/Kbuild/defconfig cleanup only. It added no non-config source
compatibility code; the temporary compatibility ledger records that explicitly
for later rollback review.

Validated through 2026-05-05 current HEAD (`ac031758aaa4`) includes follow-up
whole-file and support-tree trimming after the coarse directory scan:

- disabled-config whole-file passes removed dead files across treewide, arch,
  block, and drivers areas;
- config reduction based on `common-disabled.config` removed additional
  x86_64/arm64-disabled Kconfig surface;
- `Documentation/`, root `README`, and `LICENSES/` were removed as
  non-runtime/non-validation material;
- `scripts/` and `tools/` were reduced further to the host/build helpers still
  required by the current x86_64 and arm64 kernel builds.
- a second `scripts/` and `tools/` file-level pass removed Kconfig interactive
  frontends/tests, DTC ancillary utilities, tools feature probes, and top-level
  diagnostic/maintenance scripts without changing build or runtime logic.
- a follow-up `scripts/` pass removed additional non-target entry scripts for
  sysctl documentation checks, extable diagnostics, install-only flows,
  namespace dependency generation, object diffing, U-Boot image wrapping, and
  Xen hypercall headers.

### Stage 6: Whole-File Stragglers

Status: active

Whole-file trimming has started after the latest coarse directory passes.
Recent validated commits removed files gated by disabled config symbols and
non-essential build/support scripts. Continue preferring whole-directory
candidates first when a safe one is found; otherwise continue with whole-file
stragglers before file-internal surface reduction.

Current post-`ac031758aaa4` execution priority is grouped into three source
commits:

1. Debugfs commit: evaluate `fs/debugfs/` with a full `DEBUG_FS` dependency
   ledger before deletion. It is outside the storage contract but has many
   conditional users and config edges, so it gets its own validation boundary.
2. Drivers commit: remove `drivers/gpio/Kconfig`, `drivers/idle/`, and stale
   Kconfig/Makefile wiring. Additional unbuilt non-storage driver subtrees may
   be included only if the same build-object inventory proves they are absent
   from both fixed validation builds.
3. Arm64 commit: reduce the remaining arm64 module-only platform driver/helper
   requests as one arm64-focused change, deleting implementation files or
   directories that become unbuilt after the config cleanup.

Keep `fs/pstore/` for panic log persistence/debuggability. Do not include it in
the deletion queue unless the project goal changes.

Keep `drivers/iommu/`, `drivers/of/`, `drivers/pci/`, `drivers/nvme/`,
`drivers/scsi/`, `block/`, `fs/minix/`, `fs/ramfs/`, `fs/kernfs/`,
`fs/sysfs/`, `init/`, and `usr/` out of broad deletion attempts until a focused
dependency experiment proves they are not needed by the fixed storage contract.

### Stage 7: File-Internal Surface Reduction

Only after stages 1-6 are stable, review remaining core files for internal
deletion. This includes individual functions, tables, declarations, config
cases, and other line-level minimization. Any change here is high risk and must
be pure dead-code deletion or dead-reference cleanup. Logic changes stop the
task.

## Per-round checklist

Each normal round must:

1. Read `.codex-qemu-kernels/state.json` and this roadmap.
2. Select the largest safe directory-level deletion target available. If none
   exists, select the largest safe whole-file deletion target. Do not select
   file-internal trimming until directory and whole-file targets are exhausted.
3. Write a dependency ledger in `.codex-qemu-kernels/reviews/`.
4. Make source changes in the kernel tree.
5. Classify the patch as `pure_delete`, `delete_plus_build_wiring`,
   `move_only`, or `logic_change`.
6. Run required x86_64 and arm64 validation.
7. Update `.codex-qemu-kernels/state.json`.
8. Commit kernel source changes only, with a clear module/feature removal
   commit message.
9. Review the commit.
10. Continue unless a real allowed stop condition is hit.

Clean commits, clean validation, and clean review are recovery checkpoints, not
completion conditions.
