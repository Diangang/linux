# SPDX-License-Identifier: GPL-2.0
#
# Makefile for some libs needed in the kernel.
#

# These files are disabled because they produce lots of non-interesting and/or
# flaky coverage that is not a function of syscall inputs. For example,
# rbtree can be global and individual rotations don't correlate with inputs.

# string.o implements standard library functions like memset/memcpy etc.
# Use -ffreestanding to ensure that the compiler does not try to "optimize"
# them into calls to themselves.
CFLAGS_string.o := -ffreestanding

lib-y := ctype.o string.o vsprintf.o cmdline.o \
	 rbtree.o radix-tree.o timerqueue.o xarray.o \
	 maple_tree.o idr.o extable.o irq_regs.o argv_split.o \
	 flex_proportions.o ratelimit.o \
	 is_single_threaded.o plist.o decompress.o kobject_uevent.o \
	 earlycpio.o seq_buf.o siphash.o dec_and_lock.o \
	 nmi_backtrace.o win_minmax.o memcat_p.o \
	 buildid.o objpool.o iomem_copy.o sys_info.o

lib-$(CONFIG_PRINTK) += dump_stack.o
lib-$(CONFIG_SMP) += cpumask.o

lib-y	+= kobject.o klist.o
obj-y	+= lockref.o

obj-y += bcd.o sort.o parser.o debug_locks.o random32.o \
	 bust_spinlocks.o kasprintf.o bitmap.o scatterlist.o \
	 list_sort.o uuid.o iov_iter.o clz_ctz.o \
	 bsearch.o find_bit.o llist.o lwq.o memweight.o kfifo.o \
	 percpu-refcount.o rhashtable.o base64.o \
	 once.o refcount.o rcuref.o usercopy.o errseq.o bucket_locks.o \
	 generic-radix-tree.o bitmap-str.o
obj-y += string_helpers.o
obj-y += hexdump.o
obj-y += kstrtox.o

obj-y += math/ crc/ crypto/ vdso/

obj-$(CONFIG_GENERIC_IOMAP) += iomap.o
obj-$(CONFIG_HAS_IOMEM) += iomap_copy.o devres.o

obj-$(CONFIG_GENERIC_CPU_CACHE_MAINTENANCE) += cache_maint.o

lib-y += logic_pio.o

obj-$(CONFIG_GENERIC_HWEIGHT) += hweight.o

obj-$(CONFIG_INTERVAL_TREE) += interval_tree.o

obj-$(CONFIG_BITREVERSE) += bitrev.o
obj-$(CONFIG_GENERIC_ALLOCATOR) += genalloc.o

obj-$(CONFIG_SMP) += percpu_counter.o

lib-$(CONFIG_GENERIC_BUG) += bug.o

obj-$(CONFIG_HAVE_ARCH_TRACEHOOK) += syscall.o


obj-$(CONFIG_GENERIC_CSUM) += checksum.o

obj-$(CONFIG_GENERIC_STRNCPY_FROM_USER) += strncpy_from_user.o
obj-$(CONFIG_GENERIC_STRNLEN_USER) += strnlen_user.o

obj-$(CONFIG_SG_POOL) += sg_pool.o
libfdt_files = fdt.o fdt_ro.o fdt_wip.o fdt_rw.o fdt_sw.o fdt_strerror.o \
	       fdt_empty_tree.o fdt_addresses.o
$(foreach file, $(libfdt_files), \
	$(eval CFLAGS_$(file) = -I $(srctree)/scripts/dtc/libfdt))
lib-$(CONFIG_LIBFDT) += $(libfdt_files)

obj-$(CONFIG_UCS2_STRING) += ucs2_string.o

obj-$(CONFIG_SBITMAP) += sbitmap.o

obj-y += group_cpus.o

obj-$(CONFIG_GENERIC_LIB_DEVMEM_IS_ALLOWED) += devmem_is_allowed.o

obj-$(CONFIG_FIRMWARE_TABLE) += fw_table.o
