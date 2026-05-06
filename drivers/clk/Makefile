# SPDX-License-Identifier: GPL-2.0
# common clock types
obj-$(CONFIG_HAVE_CLK)		+= clk-devres.o clk-bulk.o clkdev.o
obj-$(CONFIG_COMMON_CLK)	+= clk.o
obj-$(CONFIG_COMMON_CLK)	+= clk-divider.o
obj-$(CONFIG_COMMON_CLK)	+= clk-fixed-factor.o
obj-$(CONFIG_COMMON_CLK)	+= clk-fixed-rate.o
obj-$(CONFIG_COMMON_CLK)	+= clk-gate.o
obj-$(CONFIG_COMMON_CLK)	+= clk-mux.o
obj-$(CONFIG_COMMON_CLK)	+= clk-fractional-divider.o
ifeq ($(CONFIG_OF), y)
obj-$(CONFIG_COMMON_CLK)	+= clk-conf.o
endif
