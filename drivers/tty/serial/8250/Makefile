# SPDX-License-Identifier: GPL-2.0
#
# Makefile for the 8250 serial device drivers.
#

obj-$(CONFIG_SERIAL_8250)		+= 8250.o
8250-y					:= 8250_core.o
8250-y					+= 8250_platform.o
8250-$(CONFIG_SERIAL_8250_PNP)		+= 8250_pnp.o

obj-$(CONFIG_SERIAL_8250)		+= 8250_base.o
8250_base-y				:= 8250_port.o
8250_base-$(CONFIG_SERIAL_8250_PCILIB)	+= 8250_pcilib.o

obj-$(CONFIG_SERIAL_8250_CONSOLE)	+= 8250_early.o

obj-$(CONFIG_SERIAL_OF_PLATFORM)	+= 8250_of.o
obj-$(CONFIG_SERIAL_8250_PCI)		+= 8250_pci.o
