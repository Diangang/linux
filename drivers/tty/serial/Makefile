# SPDX-License-Identifier: GPL-2.0
#
# Makefile for the kernel serial device drivers.
#

obj-$(CONFIG_SERIAL_CORE) += serial_base.o
serial_base-y := serial_core.o serial_base_bus.o serial_ctrl.o serial_port.o

obj-$(CONFIG_SERIAL_EARLYCON) += earlycon.o

# Now bring in any enabled 8250/16450/16550 type drivers.
obj-y += 8250/

obj-$(CONFIG_SERIAL_AMBA_PL011)		+= amba-pl011.o
obj-$(CONFIG_SERIAL_FSL_LINFLEXUART)	+= fsl_linflexuart.o
obj-$(CONFIG_SERIAL_FSL_LPUART)		+= fsl_lpuart.o
obj-$(CONFIG_SERIAL_XILINX_PS_UART)	+= xilinx_uartps.o

# GPIOLIB helpers for modem control lines
obj-$(CONFIG_SERIAL_MCTRL_GPIO)	+= serial_mctrl_gpio.o
