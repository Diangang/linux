# SPDX-License-Identifier: GPL-2.0
obj-$(CONFIG_TTY)		+= tty_io.o n_tty.o tty_ioctl.o tty_ldisc.o \
				   tty_buffer.o tty_port.o tty_mutex.o \
				   tty_ldsem.o tty_baudrate.o tty_jobctrl.o \
				   n_null.o
obj-$(CONFIG_LEGACY_PTYS)	+= pty.o
obj-$(CONFIG_UNIX98_PTYS)	+= pty.o
obj-$(CONFIG_MAGIC_SYSRQ)	+= sysrq.o

obj-y				+= serial/
obj-$(CONFIG_SERIAL_DEV_BUS)	+= serdev/
