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
