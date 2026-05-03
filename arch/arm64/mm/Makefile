# SPDX-License-Identifier: GPL-2.0
obj-y				:= dma-mapping.o extable.o fault.o init.o \
				   cache.o copypage.o flush.o \
				   ioremap.o mmap.o pgd.o mem_encrypt.o mmu.o \
				   context.o proc.o pageattr.o fixmap.o
obj-$(CONFIG_ARM64_CONTPTE)	+= contpte.o
obj-$(CONFIG_PTDUMP)		+= ptdump.o
obj-$(CONFIG_TRANS_TABLE)	+= trans_pgd.o
obj-$(CONFIG_TRANS_TABLE)	+= trans_pgd-asm.o
obj-$(CONFIG_ARM64_GCS)		+= gcs.o
KASAN_SANITIZE_physaddr.o	+= n

KASAN_SANITIZE_kasan_init.o	:= n
