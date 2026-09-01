# SPDX-License-Identifier: GPL-2.0
#
# Makefile for the linux memory manager.
#

# These files are disabled because they produce non-interesting and/or
# flaky coverage that is not a function of syscall inputs. E.g. slab is out of
# free pages, or a task is migrated between nodes.

CFLAGS_init-mm.o += -Wno-override-init

mmu-y			:= highmem.o memory.o mincore.o \
			   mlock.o mmap.o mmu_gather.o mprotect.o mremap.o \
			   msync.o page_vma_mapped.o pagewalk.o \
			   pgtable-generic.o rmap.o vmalloc.o vma.o vma_exec.o


ifdef CONFIG_64BIT
mmu-y			+= mseal.o
endif

obj-y			:= filemap.o mempool.o oom_kill.o fadvise.o \
			   maccess.o page-writeback.o folio-compat.o \
			   readahead.o swap.o truncate.o vmscan.o shrinker.o \
			   shmem.o util.o mmzone.o vmstat.o backing-dev.o \
			   mm_init.o percpu.o slab_common.o \
			   compaction.o show_mem.o \
			   interval_tree.o list_lru.o workingset.o \
			   debug.o gup.o mmap_lock.o vma_init.o $(mmu-y)

# Give 'page_alloc' its own module-parameter namespace
page-alloc-y := page_alloc.o

# Give 'memory_hotplug' its own module-parameter namespace

obj-y += page-alloc.o
obj-y += page_frag_cache.o
obj-y += init-mm.o
obj-y += memblock.o
obj-y += slub.o

obj-$(CONFIG_HAS_DMA)	+= dmapool.o
obj-$(CONFIG_NUMA) 	+= mempolicy.o
obj-$(CONFIG_SPARSEMEM)	+= sparse.o
obj-$(CONFIG_SPARSEMEM_VMEMMAP) += sparse-vmemmap.o
obj-$(CONFIG_MIGRATION) += migrate.o
obj-$(CONFIG_NUMA) += memory-tiers.o
obj-$(CONFIG_MEMORY_FAILURE) += memory-failure.o
obj-$(CONFIG_GENERIC_EARLY_IOREMAP) += early_ioremap.o
obj-$(CONFIG_NUMA) += numa.o
obj-$(CONFIG_NUMA_MEMBLKS) += numa_memblks.o
obj-$(CONFIG_GENERIC_IOREMAP) += ioremap.o
obj-$(CONFIG_EXECMEM) += execmem.o
