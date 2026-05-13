# SPDX-License-Identifier: GPL-2.0
# Makefile for the Linux firmware loader

obj-$(CONFIG_FW_LOADER)	+= firmware_class.o
firmware_class-objs := main.o
firmware_class-$(CONFIG_FW_LOADER_SYSFS) += sysfs.o

obj-y += builtin/
