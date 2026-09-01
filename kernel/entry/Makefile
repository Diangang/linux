# SPDX-License-Identifier: GPL-2.0

# Prevent the noinstr section from being pestered by sanitizer and other goodies
# as long as these things cannot be disabled per function.

CFLAGS_REMOVE_common.o	 = -fstack-protector -fstack-protector-strong
CFLAGS_common.o		+= -fno-stack-protector

obj-$(CONFIG_GENERIC_IRQ_ENTRY) 	+= common.o
obj-$(CONFIG_GENERIC_SYSCALL) 		+= syscall_user_dispatch.o
