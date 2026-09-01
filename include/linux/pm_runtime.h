/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * pm_runtime.h - Device run-time power management helper functions.
 *
 * Copyright (C) 2009 Rafael J. Wysocki <rjw@sisk.pl>
 */

#ifndef _LINUX_PM_RUNTIME_H
#define _LINUX_PM_RUNTIME_H

#include <linux/device.h>
#include <linux/pm.h>

/* Runtime PM flag argument bits */
#define RPM_ASYNC		0x01	/* Request is asynchronous */
#define RPM_GET_PUT		0x04	/* Increment/decrement the
						    usage_count */
#define RPM_TRANSPARENT	0x10	/* Succeed if runtime PM is disabled */

static inline int __pm_runtime_idle(struct device *dev, int rpmflags)
{
	return -ENOSYS;
}
static inline int __pm_runtime_resume(struct device *dev, int rpmflags)
{
	return 1;
}
static inline int __pm_runtime_set_status(struct device *dev,
					    unsigned int status) { return 0; }
static inline void pm_runtime_barrier(struct device *dev) {}
static inline void pm_runtime_forbid(struct device *dev) {}

static inline void pm_runtime_get_noresume(struct device *dev) {}
static inline void pm_runtime_put_noidle(struct device *dev) {}

/**
 * pm_runtime_resume - Resume a device synchronously.
 * @dev: Target device.
 */
static inline int pm_runtime_resume(struct device *dev)
{
	return __pm_runtime_resume(dev, 0);
}

/**
 * pm_runtime_get_sync - Bump up usage counter of a device and resume it.
 * @dev: Target device.
 *
 * Bump up the runtime PM usage counter of @dev and carry out runtime-resume of
 * it synchronously.
 *
 * The possible return values of this function are the same as for
 * pm_runtime_resume() and the runtime PM usage counter of @dev remains
 * incremented in all cases, even if it returns an error code.
 */
static inline int pm_runtime_get_sync(struct device *dev)
{
	return __pm_runtime_resume(dev, RPM_GET_PUT);
}

static inline int pm_runtime_get_active(struct device *dev, int rpmflags)
{
	int ret;

	ret = __pm_runtime_resume(dev, RPM_GET_PUT | rpmflags);
	if (ret < 0) {
		pm_runtime_put_noidle(dev);
		return ret;
	}

	return 0;
}

/**
 * pm_runtime_put - Drop device usage counter and queue up "idle check" if 0.
 * @dev: Target device.
 *
 * Decrement the runtime PM usage counter of @dev and if it turns out to be
 * equal to 0, queue up an asynchronous idle request for @dev.
 */
static inline void pm_runtime_put(struct device *dev)
{
	__pm_runtime_idle(dev, RPM_GET_PUT | RPM_ASYNC);
}

DEFINE_GUARD(pm_runtime_active, struct device *,
	     pm_runtime_get_sync(_T), pm_runtime_put(_T));
/*
 * Use the following guards with ACQUIRE()/ACQUIRE_ERR().
 *
 * The difference between the "_try" and "_try_enabled" variants is that the
 * former do not produce an error when runtime PM is disabled for the given
 * device.
 */
DEFINE_GUARD_COND(pm_runtime_active, _try,
		  pm_runtime_get_active(_T, RPM_TRANSPARENT), _RET == 0)

/* ACQUIRE() wrapper macros for the guards defined above. */

#define PM_RUNTIME_ACQUIRE(_dev, _var)			\
	ACQUIRE(pm_runtime_active_try, _var)(_dev)

/*
 * ACQUIRE_ERR() wrapper macro for guard pm_runtime_active.
 *
 * Always check PM_RUNTIME_ACQUIRE_ERR() after using one of the
 * PM_RUNTIME_ACQUIRE*() macros defined above (yes, it can be used with
 * any of them) and if it is nonzero, avoid accessing the given device.
 */
#define PM_RUNTIME_ACQUIRE_ERR(_var_ptr)	\
	ACQUIRE_ERR(pm_runtime_active, _var_ptr)

/**
 * pm_runtime_put_sync - Drop device usage counter and run "idle check" if 0.
 * @dev: Target device.
 *
 * Decrement the runtime PM usage counter of @dev and if it turns out to be
 * equal to 0, invoke the "idle check" callback of @dev and, depending on its
 * return value, set up autosuspend of @dev or suspend it (depending on whether
 * or not autosuspend has been enabled for it).
 *
 * The runtime PM usage counter of @dev remains decremented in all cases, even
 * if it returns an error code.
 *
 * Return:
 * * 1: Success. Usage counter dropped to zero, but device was already suspended.
 * * 0: Success.
 * * -EINVAL: Runtime PM error.
 * * -EACCES: Runtime PM disabled.
 * * -EAGAIN: Runtime PM usage counter became non-zero or Runtime PM status
 *            change ongoing.
 * * -EBUSY: Runtime PM child_count non-zero.
 * * -EPERM: Device PM QoS resume latency 0.
 * * -ENOSYS: CONFIG_PM not enabled.
 * Other values and conditions for the above values are possible as returned by
 * Runtime PM suspend callbacks.
 */
static inline int pm_runtime_put_sync(struct device *dev)
{
	return __pm_runtime_idle(dev, RPM_GET_PUT);
}

/**
 * pm_runtime_set_active - Set runtime PM status to "active".
 * @dev: Target device.
 *
 * Set the runtime PM status of @dev to %RPM_ACTIVE and ensure that dependencies
 * of it will be taken into account.
 *
 * It is not valid to call this function for devices with runtime PM enabled.
 */
static inline int pm_runtime_set_active(struct device *dev)
{
	return __pm_runtime_set_status(dev, RPM_ACTIVE);
}

#endif
