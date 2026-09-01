/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 *  pm_wakeup.h - Power management wakeup interface
 *
 *  Copyright (C) 2008 Alan Stern
 *  Copyright (C) 2010 Rafael J. Wysocki, Novell Inc.
 */

#ifndef _LINUX_PM_WAKEUP_H
#define _LINUX_PM_WAKEUP_H

#ifndef _DEVICE_H_
# error "Please do not include this file directly."
#endif

#include <linux/types.h>

struct wake_irq;

/**
 * struct wakeup_source - Representation of wakeup sources
 *
 * @name: Name of the wakeup source
 * @id: Wakeup source id
 * @entry: Wakeup source list entry
 * @lock: Wakeup source lock
 * @wakeirq: Optional device specific wakeirq
 * @timer: Wakeup timer list
 * @timer_expires: Wakeup timer expiration
 * @total_time: Total time this wakeup source has been active.
 * @max_time: Maximum time this wakeup source has been continuously active.
 * @last_time: Monotonic clock when the wakeup source's was touched last time.
 * @prevent_sleep_time: Total time this source has been preventing autosleep.
 * @event_count: Number of signaled wakeup events.
 * @active_count: Number of times the wakeup source was activated.
 * @relax_count: Number of times the wakeup source was deactivated.
 * @expire_count: Number of times the wakeup source's timeout has expired.
 * @wakeup_count: Number of times the wakeup source might abort suspend.
 * @dev: Struct device for sysfs statistics about the wakeup source.
 * @active: Status of the wakeup source.
 * @autosleep_enabled: Autosleep is active, so update @prevent_sleep_time.
 */
struct wakeup_source {
	const char 		*name;
	int			id;
	struct list_head	entry;
	spinlock_t		lock;
	struct wake_irq		*wakeirq;
	struct timer_list	timer;
	unsigned long		timer_expires;
	ktime_t total_time;
	ktime_t max_time;
	ktime_t last_time;
	ktime_t start_prevent_time;
	ktime_t prevent_sleep_time;
	unsigned long		event_count;
	unsigned long		active_count;
	unsigned long		relax_count;
	unsigned long		expire_count;
	unsigned long		wakeup_count;
	struct device		*dev;
	bool			active:1;
	bool			autosleep_enabled:1;
};

static inline void device_set_wakeup_capable(struct device *dev, bool capable)
{
	dev->power.can_wakeup = capable;
}

static inline bool device_can_wakeup(struct device *dev)
{
	return dev->power.can_wakeup;
}

static inline int device_wakeup_enable(struct device *dev)
{
	dev->power.should_wakeup = true;
	return 0;
}

static inline void device_wakeup_disable(struct device *dev)
{
	dev->power.should_wakeup = false;
}

static inline bool device_may_wakeup(struct device *dev)
{
	return dev->power.can_wakeup && dev->power.should_wakeup;
}

static inline void device_set_wakeup_path(struct device *dev) {}

static inline void pm_wakeup_dev_event(struct device *dev, unsigned int msec,
				       bool hard) {}

static inline void device_set_awake_path(struct device *dev)
{
	device_set_wakeup_path(dev);
}

static inline void pm_wakeup_event(struct device *dev, unsigned int msec)
{
	pm_wakeup_dev_event(dev, msec, false);
}

#endif /* _LINUX_PM_WAKEUP_H */
