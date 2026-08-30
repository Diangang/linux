/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Per-device information from the pin control system.
 * This is the stuff that get included into the device
 * core.
 *
 * Copyright (C) 2012 ST-Ericsson SA
 * Written on behalf of Linaro for ST-Ericsson
 * This interface is used in the core to keep track of pins.
 *
 * Author: Linus Walleij <linus.walleij@linaro.org>
 */

#ifndef PINCTRL_DEVINFO_H
#define PINCTRL_DEVINFO_H

struct device;


/* Stubs if we're not using pinctrl */

static inline int pinctrl_init_done(struct device *dev)
{
	return 0;
}

static inline struct pinctrl *dev_pinctrl(struct device *dev)
{
	return NULL;
}

#endif /* PINCTRL_DEVINFO_H */
