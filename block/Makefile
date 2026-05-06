# SPDX-License-Identifier: GPL-2.0
#
# Makefile for the kernel block layer
#

obj-y		:= bdev.o fops.o bio.o elevator.o blk-core.o blk-sysfs.o \
			blk-flush.o blk-settings.o blk-ioc.o blk-map.o \
			blk-merge.o blk-timeout.o blk-lib.o blk-mq.o \
			blk-mq-tag.o blk-mq-dma.o blk-stat.o \
			blk-mq-sysfs.o blk-mq-cpumap.o blk-mq-sched.o ioctl.o \
			genhd.o ioprio.o badblocks.o partitions/ blk-rq-qos.o \
			disk-events.o blk-ia-ranges.o early-lookup.o

obj-$(CONFIG_BLK_CGROUP)	+= blk-cgroup.o
obj-$(CONFIG_BLK_CGROUP_RWSTAT)	+= blk-cgroup-rwstat.o
obj-$(CONFIG_BLK_CGROUP_IOPRIO)	+= blk-ioprio.o
obj-$(CONFIG_BLK_CGROUP_IOLATENCY)	+= blk-iolatency.o
obj-$(CONFIG_BLK_CGROUP_IOCOST)	+= blk-iocost.o
obj-$(CONFIG_MQ_IOSCHED_DEADLINE)	+= mq-deadline.o
obj-$(CONFIG_MQ_IOSCHED_KYBER)	+= kyber-iosched.o
bfq-y				:= bfq-iosched.o bfq-wf2q.o bfq-cgroup.o
obj-$(CONFIG_IOSCHED_BFQ)	+= bfq.o

obj-$(CONFIG_BLK_PM)		+= blk-pm.o
obj-$(CONFIG_BLK_INLINE_ENCRYPTION)	+= blk-crypto.o blk-crypto-profile.o \
					   blk-crypto-sysfs.o
