/* SPDX-License-Identifier: GPL-2.0 */
/*
 * NOTE: This header *must not* be included.
 *
 * This is the LEGACY GPIO bulk include file, including legacy APIs. It is
 * used for GPIO drivers still referencing the global GPIO numberspace,
 * and should not be included in new code.
 *
 * If you're implementing a GPIO driver, only include <linux/gpio/driver.h>
 * If you're implementing a GPIO consumer, only include <linux/gpio/consumer.h>
 */
#ifndef __LINUX_GPIO_H
#define __LINUX_GPIO_H

#include <linux/types.h>

#ifdef CONFIG_GPIOLIB_LEGACY

struct device;

/* make these flag values available regardless of GPIO kconfig options */
#define GPIOF_IN		((1 << 0))
#define GPIOF_OUT_INIT_LOW	((0 << 0) | (0 << 1))
#define GPIOF_OUT_INIT_HIGH	((0 << 0) | (1 << 1))


#include <linux/kernel.h>

#include <asm/bug.h>
#include <asm/errno.h>

static inline bool gpio_is_valid(int number)
{
	return false;
}

static inline int gpio_request(unsigned gpio, const char *label)
{
	return -ENOSYS;
}

static inline int gpio_request_one(unsigned gpio,
					unsigned long flags, const char *label)
{
	return -ENOSYS;
}

static inline void gpio_free(unsigned gpio)
{
	might_sleep();

	/* GPIO can never have been requested */
	WARN_ON(1);
}

static inline int gpio_direction_input(unsigned gpio)
{
	return -ENOSYS;
}

static inline int gpio_direction_output(unsigned gpio, int value)
{
	return -ENOSYS;
}

static inline int gpio_get_value(unsigned gpio)
{
	/* GPIO can never have been requested or set as {in,out}put */
	WARN_ON(1);
	return 0;
}

static inline void gpio_set_value(unsigned gpio, int value)
{
	/* GPIO can never have been requested or set as output */
	WARN_ON(1);
}

static inline int gpio_get_value_cansleep(unsigned gpio)
{
	/* GPIO can never have been requested or set as {in,out}put */
	WARN_ON(1);
	return 0;
}

static inline void gpio_set_value_cansleep(unsigned gpio, int value)
{
	/* GPIO can never have been requested or set as output */
	WARN_ON(1);
}

static inline int gpio_to_irq(unsigned gpio)
{
	/* GPIO can never have been requested or set as input */
	WARN_ON(1);
	return -EINVAL;
}

static inline int devm_gpio_request_one(struct device *dev, unsigned gpio,
					unsigned long flags, const char *label)
{
	WARN_ON(1);
	return -EINVAL;
}

#endif /* CONFIG_GPIOLIB_LEGACY */
#endif /* __LINUX_GPIO_H */
