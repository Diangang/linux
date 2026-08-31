# SPDX-License-Identifier: GPL-2.0
#
# Makefile for the x86 low level entry code
#

UBSAN_SANITIZE := n
KCOV_INSTRUMENT := n

CFLAGS_syscall_64.o		+= -fno-stack-protector

obj-y				:= entry.o entry_$(BITS).o syscall_$(BITS).o

obj-y				+= vdso/
obj-$(CONFIG_PREEMPTION)	+= thunk.o
