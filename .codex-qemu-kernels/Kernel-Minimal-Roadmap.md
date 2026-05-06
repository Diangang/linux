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

Current Stage 6 source progress as of 2026-05-07:

- Done through `e8492b4d13ff`: fixed-target debugfs, no-fixed-object GPIO/idle
  leaves, arm64 module residue, multiple clock/firmware/irqchip/support-tree
  proof passes, ending with EFI unaccepted memory support removal.
- Done after the task-state catch-up: `8926549c6cb5` OF test/PROMTREE support,
  `17357f509ffe` non-target block drivers, `302f0f34791d` non-PCI NVMe host
  support, `1c00a77564e1` SCSI proc/bsg interfaces, `8058c5d4f79c` PCI hotplug
  and endpoint support, `5bb009036052` unused clock helpers,
  `5be1a4a08b76` MMIO arch timer support, `4b5c8f8cf28e` DMA engine drivers,
  `f23ffb900543` IOMMU hardware support, `e5159dd11ab5` unused SoC bus drivers,
  `690c439ebdbb` non-essential character devices, `99e5d23b1fe7` unused pstore
  backends, `5fb81871c34a` unused x86 subarchitecture support,
  `aad41b0a6a34` unused arm64 architecture extensions, `40873a30668f` BPF
  subsystem support, `0931023c96dd` unused helper libraries, `5af24005e903`
  unused block-layer interfaces and partition support, `356ede1b5f3e` unused MM
  debug/zsmalloc support, `f3d8d92acfaf` unused initramfs test/fallback support,
  `0e1a4dea5ee9` UAPI header test support, `50af0c25dabf` optional ACPI leaves,
  `aecce9cf45eb` Tegra AHB AMBA support, `8a2c4e5a596f` driver core
  test/debug leaves, `d72f4fa8f801` unused TTY/serial drivers, and
  `32bbdffde87e` unused virtio leaf drivers.
- Current validated HEAD: `069efd8ed1b6`. Latest fixed contract logs contain
  `CODEX_MINIX_TEST_PASS` for both x86_64 and arm64.
- Latest include/header-family progress: `1947a15d1c68` raid UAPI headers,
  `f9eb1e500b33` tc action headers, `ce036c5e9ebb` Ceph headers,
  `6a23ce2683d4` non-target include/net families, `0fc98e2e95c6` PSP/SCTP
  headers, `b47b12979328` firmware header family plus tightly-coupled upper
  sound/MFD headers, and `d8b910bd0429` zero-dependency `dt-bindings` child
  directories, and `11a745585a1e` zero-dependency DRM child header
  directories, and `6cf49c9039ca` zero-dependency media child header
  directories, `e87def00d5f8` zero-dependency Xen UAPI headers, and
  `795f57b63e10` zero-dependency Intel DRM headers, `d7e2037a4e76`
  zero-dependency RDMA headers, `2d317c8356c8` zero-dependency sound headers,
  `27d54389b46a` zero-dependency netfilter headers, `e97f32920162`
  zero-dependency IIO headers, `dca137c4f656` zero-dependency DRM TTM headers,
  `ce2eb71a234d` zero-dependency media driver interface headers,
  `38fe4552d3c8` zero-dependency PHY binding headers, `ec00edee744d`
  zero-dependency TI SoC/K3 headers, `ddcf02e8c4c0` zero-dependency WM831x
  MFD headers, `c20a50acbd1e` zero-dependency interrupt-controller DT binding
  headers, `bbcf4af69827` zero-dependency WM8350 MFD headers,
  `803b70739abd` zero-dependency Mediatek SoC headers, `962142cd2324`
  zero-dependency ipset header family, `ee5b63aa2765` zero-dependency
  SoundWire header family plus regmap support, `56891fd30c30`
  zero-dependency media platform-data header directory, `914e89df8600`
  zero-dependency Samsung SoC header directory, `c98adf28ce00`
  zero-dependency WM8994 MFD header directory, `435d4609603a`
  zero-dependency SPI offload header family, `00d554984b11`
  zero-dependency Renesas SoC header directory, `50715e010677`
  zero-dependency PXA SoC header directory, `bca791f0f092`
  zero-dependency IXP4XX SoC header directory, `04fff8be5307`
  zero-dependency Apple SoC and MacSMC header family, `2edd90f73752`
  zero-dependency MT6397 MFD header directory, `6aeaa3bda543`
  zero-dependency Madera MFD header family, `01d6c19da63d`
  zero-dependency DA9055 MFD header directory, `9c3cd8058d33`
  zero-dependency DA9052 MFD header directory, `cba6b2d2a7bd`
  zero-dependency ATC260x MFD header directory, `20790d8a64fd`
  zero-dependency Arizona MFD header family, `b450d88f18ed`
  zero-dependency ABx500 MFD header family, `3f87de6e3e08`
  zero-dependency WCD934x MFD header directory, `1e85c521b2fc`
  zero-dependency Mediatek PMIC header family, `20eb93edc737`
  zero-dependency Dialog PMIC header family, `279765e48afd`,
  `73d4ada230fa` zero-dependency DMA/sync helper header family, and
  `21da8d6d48af` zero-dependency USB leaf header family, and `03b38d6fce0e`
  zero-dependency nfnetlink header family, and `39dd292cb903`
  zero-dependency USB-adjacent/OMAP header family, and `22035d8cc825`
  zero-dependency USB residual header family, and `6c52033fc509`
  zero-dependency USB OTG header, `d4af0e8c77f7` zero-dependency media leaf
  headers, `9de5664f88c0` zero-dependency SCSI leaf/transport headers, and
  `08e7dda4eafe` zero-dependency Xen leaf headers, and `efe7759cc0f4`
  zero-dependency USB PHY header, and `e105d0078882` zero-dependency residual
  media leaf headers, and `069efd8ed1b6` zero-dependency residual SCSI headers
  zero-dependency Marvell SoC header directory, `3aa1e3d24ce5`
  zero-dependency Airoha SoC header directory, `22ce87dee945`
  zero-dependency display DT binding header directory, `bb44c75de1e4`
  zero-dependency Sunxi SoC header directory, `a9719b685380`
  zero-dependency NXP SoC header directory, `9064c92b1451`
  zero-dependency MMP SoC header directory, `00dd447bb7a0`
  zero-dependency Actions SoC header directory, `9a58740fea84`
  zero-dependency AMD SoC header directory, `0bd059b723f3`
  zero-dependency Amlogic SoC header directory, `6975426ccd19`
  zero-dependency Andes SoC header directory, `3c6152d8d806`
  zero-dependency BRCMSTB SoC header directory, `e07d85ec5d17`
  zero-dependency Cirrus SoC header directory, `0dcc076b25db`
  zero-dependency Dove SoC header directory, `83e6a564a9b3`
  zero-dependency PMU DT binding header directory, `9c8bf62c3f9f`
  zero-dependency LED DT binding header family, `1dd875ceeb57`
  zero-dependency AMD FCH GPIO platform-data header directory,
  `11edb175eac9` zero-dependency Tegra XUSB PHY header directory,
  `7c5495fbeea8` zero-dependency TXX9 NAND platform-data header directory, and
  `9fcaba93ea48` zero-dependency Intel DRM residual header family, and
  `6a2778952537` zero-dependency x86 platform-data residual header family,
  `7553746f8fe3` zero-dependency AMBA peripheral residual header family,
  `8bf3f41e2f87` zero-dependency io_uring side UAPI header family,
  `ce997f59f726` zero-dependency RISC-V PMU header,
  `51f69646af67` zero-dependency RAID helper headers, and
  `82dd37792c63` zero-dependency vendor clock headers, and
  `af2d25b74349` zero-dependency vendor DMA headers, and
  `397157df15b4` zero-dependency MFD headers, and
  `9fb73ab64a2e` zero-dependency platform_data headers, and
  `72e021578e17` zero-dependency syscon headers,
  `8e7c73c10c23` zero-dependency Intel SPI platform header,
  `8320a0e31c69` zero-dependency pinctrl headers,
  `13f5652888d1` zero-dependency SSB leaf headers,
  `591e6db74172` zero-dependency KHO metadata header,
  `7cffafb95d9c` zero-dependency ARM PMU header,
  `f857b1c61f5b` zero-dependency irqchip headers,
  `5b27bacd3617` zero-dependency netfilter leaf headers,
  `a021642104fb` zero-dependency io_uring network header,
  `a822609f532f` zero-dependency Apple ADB/CUDA/PMU headers,
  `55178838f53a` zero-dependency VIA device headers,
  `0a4020c1f5e4` zero-dependency HID header family,
  `9459b4169ef6` zero-dependency ATM/AppleTalk header families,
  `44ea6001c66a` zero-dependency AX.25/X.25 header family,
  `d708e595b67f` zero-dependency FDDI/FC header family,
  `a934d6dee2a2` zero-dependency ROSE header,
  `ccdfa0d167b1` zero-dependency network leaf headers,
  `6731ec62e6a9` zero-dependency netfilter UAPI leaf headers,
  `9ebf68df76b2` zero-dependency non-Minix filesystem UAPI headers,
  `cfb61791b622` zero-dependency media device UAPI headers,
  `440efa5656d8` zero-dependency device interface headers, and
  `93dc6b383dfe` zero-dependency network protocol UAPI headers, and
  `22474878d240` zero-dependency platform/firmware/virtualization UAPI
  headers, and `57328101ec5a` zero-dependency optional virtio device UAPI
  headers, and `3e1bdf153cfc` zero-dependency remaining network leaf UAPI
  headers, and `4a78181e34a6` zero-dependency miscellaneous device UAPI
  headers, and `3dc68b5f6d76` zero-dependency filesystem/block-control UAPI
  headers, `8154ec359861` zero-dependency NTB include headers,
  `29889ba3cbc7` zero-dependency NVMe-FC include headers,
  `c30c416ecb7e` zero-dependency RTSX include headers,
  `566b81ab9ea0` zero-dependency PSP/SEV include and UAPI headers,
  `3b7c9c1cbe81` zero-dependency remoteproc include header,
  `093b961c4a86` zero-dependency FSL IFC include header,
  `f61f68a67707` zero-dependency CoreSight include header,
  `34334886c5f9` zero-dependency vDPA include header,
  `f74e63bd7252` zero-dependency Broadcom PHY include header,
  `1e6823ce67af` zero-dependency Thunderbolt include header,
  `aa078320e4ef` zero-dependency F2FS header family,
  `16e20fa13d40` zero-dependency DRBD header family,
  `aeb5daad4e48` zero-dependency Counter header family,
  `997143a7241b` zero-dependency SFP include header,
  `1bf1e53120ef` zero-dependency CCP include header,
  `06e4be7e8893` zero-dependency HWMON header family,
  `f373debafc9b` zero-dependency RapidIO header family,
  `0305f436ca67` zero-dependency TEE header family,
  `f8858c8bad1e` zero-dependency Host1x header family,
  `4ea9bf5c672f` zero-dependency FSI regmap/header family,
  `17330d2f4b2d` zero-dependency LED/V4L2 flash header family,
  `b491bef6ee8c` zero-dependency devfreq residual header family,
  `e282fdb12d84` zero-dependency memstick header, `421afc6ef2ce`
  zero-dependency orphan driver interface header family, `c8700e122fef`
  zero-dependency I2C helper header family, `41cd15408463`
  zero-dependency device-mapper header family, `cabb6272edcd`
  zero-dependency SCSI UAPI header family, `f6c7e956f071`
  zero-dependency video header family, `a0d48fbe9f7e`
  zero-dependency memory leaf header, `5a382a888e17`
  zero-dependency page_pool helper headers, `16bb82d346c2`
  zero-dependency DRM residual leaf headers, `a66214eb4b75`
  zero-dependency clocksource platform headers, `bca8cfb4d0bc`
  zero-dependency SCSI FC/FCoE header family, `e32df753cabe`
  zero-dependency trace event headers, `d3f8b1125658`
  zero-dependency net probe trace helper, `643c62751a73`
  zero-dependency KUnit auxiliary headers, `553bf4f16f00`
  zero-dependency SPI auxiliary headers, `da3d6daa0c71`
  zero-dependency regulator auxiliary headers, `35f5f09ec195`
  zero-dependency GPIO auxiliary headers, `94780002e25c`
  zero-dependency keys auxiliary headers, `bdcac9c773c1`
  zero-dependency keys residual headers, `4d04f6b58461`
  zero-dependency ACPI leaf headers, `4fcae3bbeac7`
  zero-dependency crypto leaf headers, `f2b78ce3f765`
  zero-dependency DRM residual headers, `a91855d118bc`
  zero-dependency media leaf headers, `7e384a216a3f`
  zero-dependency ARM KVM leaf headers, `80edf1e6ef69`
  zero-dependency video nomodeset header, `5ca7658bcb75`
  zero-dependency crypto residual leaf headers, `935e391b6b9e`
  zero-dependency DRM residual leaf headers, `4f50b163b8e5`
  zero-dependency media residual leaf headers, `6c05c17a9786`
  zero-dependency DRM core header chain, `73622967902c`
  zero-dependency platform residual headers, `37aecd356ae7`
  zero-dependency IEEE80211 auxiliary headers, `a87303bda5ad`
  zero-dependency virtual network interface headers, `01eb04bcd610`
  zero-dependency BPF auxiliary headers, `e5386970ee3d`
  zero-dependency ACPI auxiliary headers, `5e24c3c39a1a`
  zero-dependency ATA/AHCI platform headers, `01514212fb29`
  zero-dependency Altera UART/JTAG UART headers, and `cf9b0c30024b`
  zero-dependency platform/power residual headers, and `e35fb0cbfaa8`
  zero-dependency auxiliary algorithm headers, and `b1b28b7ef5ee`
  zero-dependency LCD helper headers, and `7c7ed06cf99a`
  zero-dependency storage/MM helper headers, and `344efe822374`
  zero-dependency Broadcom platform headers, `9c827834dde2`
  zero-dependency hardware helper headers, `8ae0c4e0bb08`
  zero-dependency platform/debug helper headers, `a8e428ed3b06`
  zero-dependency legacy platform network headers, `1513dda8ae75`
  zero-dependency legacy peripheral/bus headers, `979b29e883cb`
  zero-dependency Freescale platform headers, `994d508ecd54`
  zero-dependency input/GPIO headers, `291abda58059`
  zero-dependency Intel platform headers, `4be3d3c0b633`
  zero-dependency legacy device helper headers, `ff5c346a2deb`
  zero-dependency legacy network platform headers, `96f0700bac1a`
  zero-dependency legacy Ethernet helper headers, `d4bd1bdba2e5`
  zero-dependency legacy protocol wrapper headers, `088a4c0ae66b`
  zero-dependency legacy platform helper headers, `92eae2a1bdf9`
  zero-dependency non-target filesystem headers, `f72224f1fe13`
  zero-dependency TPM/TSM helper headers, `10fe355e238e`
  zero-dependency PCI optional helper headers, `3cca50f376df`
  zero-dependency PTP helper headers, `8223c9a40373`
  zero-dependency CoreSight helper headers, `bc15c6a89d26`
  zero-dependency power management helper headers, `a1d971e0c1bc`
  zero-dependency legacy peripheral helper headers, `2439de33359f`
  zero-dependency device bus helper headers, `cbbc7bf6f40a`
  zero-dependency display/graphics helper headers, `eaccd8aedf5f`
  zero-dependency optional storage helper headers, `c9bd8bd5ca71`
  zero-dependency optional network helper headers, `683c05d6ccaf`
  zero-dependency PHY/MDIO helper headers, `223a2fffd510`
  zero-dependency legacy platform helper headers, `a99cd0b7ebed`
  zero-dependency management device interface headers, `57c315691caf`
  zero-dependency power/platform control headers, `4a42eebb3a33`
  zero-dependency AGP headers, `b7ac5e612756` zero-dependency legacy device
  interface headers, `9557bc2622b5` zero-dependency legacy platform and
  peripheral headers, `4f94a551b1ae` zero-dependency optional interface
  wrapper headers, `a3b95c9bceda` zero-dependency optional platform/debug
  headers, `7c754806aa4c` zero-dependency optional file event headers,
  `c931ab11abfb` zero-dependency optional network/protocol headers,
  `0a9e0df647fd` zero-dependency optional infrastructure headers,
  `b72ad9f51fb0` zero-dependency legacy device/helper headers,
  `05e3c6297e7e` zero-dependency miscellaneous helper headers, and
  `88345213e1eb` zero-dependency optional core-adjacent headers.
- Next: reassess the remaining include directory and file-family candidates
  from current evidence. Prefer a whole directory or tightly-coupled header
  family before any file-internal trimming.
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

Validated through 2026-05-06 current HEAD (`962142cd2324`) includes follow-up
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
- zero-dependency include directory proof passes removed RDMA, sound,
  netfilter, IIO, DRM UAPI, DRM display, crypto internal, SoC DT binding, MFD
  DT binding, Samsung MFD, Tegra SoC, Xen IO interface, Qualcomm SoC, and DRM
  TTM, media driver interface, PHY binding, TI SoC/K3, WM831x MFD,
  interrupt-controller DT binding, WM8350 MFD, Mediatek SoC, and ipset headers
  plus Intel DRM, x86 platform-data, AMBA peripheral residual headers, vendor
  clock headers, vendor DMA headers, and MFD headers that are not referenced by
  the fixed x86_64/arm64 generated build dependencies, plus platform_data
  headers, syscon headers, the Intel SPI platform header, pinctrl headers, SSB
  leaf headers, KHO metadata header, ARM PMU header, irqchip headers, and
  netfilter leaf headers, the io_uring network header, Apple ADB/CUDA/PMU
  headers, VIA device headers, the HID header family, ATM/AppleTalk header
  families, the AX.25/X.25 header family, the FDDI/FC header family, the ROSE
  header, zero-dependency network leaf headers, netfilter UAPI leaf headers,
  non-Minix filesystem UAPI headers, media device UAPI headers, device
  interface headers, network protocol UAPI headers, and platform/firmware/
  virtualization UAPI headers, and optional virtio device UAPI headers outside
  the QEMU Minix storage contract, plus remaining network interface, tunnel,
  diagnostic, metrics, optional protocol-control UAPI leaf headers, and
  miscellaneous device/test/virtual/legacy UAPI headers, and non-Minix
  filesystem plus optional block/storage-control UAPI headers, and display,
  media, and segment-map UAPI leaf headers, and remaining virtualization/control
  UAPI leaf headers, security/memory/user-control UAPI leaf headers, and legacy
  binary/device UAPI leaf headers, crypto/network UAPI leaf headers, the vhost
  types UAPI leaf header, the NTB internal include header pair, the NVMe-FC
  internal include header pair, the RTSX card-reader include header pair, the
  PSP/SEV internal plus UAPI header pair, the remoteproc include header, the
  FSL IFC include header, the CoreSight include header, the vDPA include
  header, the Broadcom PHY include header, the Thunderbolt include header, and
  the F2FS header family, the DRBD header family, the Counter header family,
  the SFP include header, the CCP include header, the HWMON header family, the
  RapidIO header family, the TEE header family, the Host1x header family, and
  the FSI regmap/header family, the LED/V4L2 flash header family, and the
  devfreq residual header family, and the memstick header.

### Stage 6: Whole-File Stragglers

Status: active

Whole-file trimming has started after the latest coarse directory passes.
Recent validated commits removed files gated by disabled config symbols and
non-essential build/support scripts. Continue preferring whole-directory
candidates first when a safe one is found; otherwise continue with whole-file
stragglers before file-internal surface reduction.

Current execution priority after `32bbdffde87e` is no longer the old three-commit
debugfs/drivers/arm64 queue. Those passes and many later module-directory proof
passes have already completed and validated. The next normal round must first
refresh the current tree inventory, fixed x86_64/arm64 configs, generated
`.cmd` dependencies, and built-object lists, then choose the largest safe
whole-directory or whole-file proof target still remaining.

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
