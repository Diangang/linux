// SPDX-License-Identifier: GPL-2.0-only
/*
 * drivers/acpi/device_pm.c - ACPI device power management routines.
 *
 * Copyright (C) 2012, Intel Corp.
 * Author: Rafael J. Wysocki <rafael.j.wysocki@intel.com>
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */

#define pr_fmt(fmt) "PM: " fmt

#include <linux/acpi.h>
#include <linux/export.h>
#include <linux/mutex.h>
#include <linux/pm_qos.h>
#include <linux/suspend.h>

#include "internal.h"

/**
 * acpi_power_state_string - String representation of ACPI device power state.
 * @state: ACPI device power state to return the string representation of.
 */
const char *acpi_power_state_string(int state)
{
	switch (state) {
	case ACPI_STATE_D0:
		return "D0";
	case ACPI_STATE_D1:
		return "D1";
	case ACPI_STATE_D2:
		return "D2";
	case ACPI_STATE_D3_HOT:
		return "D3hot";
	case ACPI_STATE_D3_COLD:
		return "D3cold";
	default:
		return "(unknown)";
	}
}

static int acpi_dev_pm_explicit_get(struct acpi_device *device, int *state)
{
	unsigned long long psc;
	acpi_status status;

	status = acpi_evaluate_integer(device->handle, "_PSC", NULL, &psc);
	if (ACPI_FAILURE(status))
		return -ENODEV;

	*state = psc;
	return 0;
}

/**
 * acpi_device_get_power - Get power state of an ACPI device.
 * @device: Device to get the power state of.
 * @state: Place to store the power state of the device.
 *
 * This function does not update the device's power.state field, but it may
 * update its parent's power.state field (when the parent's power state is
 * unknown and the device's power state turns out to be D0).
 *
 * Also, it does not update power resource reference counters to ensure that
 * the power state returned by it will be persistent and it may return a power
 * state shallower than previously set by acpi_device_set_power() for @device
 * (if that power state depends on any power resources).
 */
int acpi_device_get_power(struct acpi_device *device, int *state)
{
	int result = ACPI_STATE_UNKNOWN;
	struct acpi_device *parent;
	int error;

	if (!device || !state)
		return -EINVAL;

	parent = acpi_dev_parent(device);

	if (!device->flags.power_manageable) {
		/* TBD: Non-recursive algorithm for walking up hierarchy. */
		*state = parent ? parent->power.state : ACPI_STATE_D0;
		goto out;
	}

	/*
	 * Get the device's power state from power resources settings and _PSC,
	 * if available.
	 */
	if (device->power.flags.power_resources) {
		error = acpi_power_get_inferred_state(device, &result);
		if (error)
			return error;
	}
	if (device->power.flags.explicit_get) {
		int psc;

		error = acpi_dev_pm_explicit_get(device, &psc);
		if (error)
			return error;

		/*
		 * The power resources settings may indicate a power state
		 * shallower than the actual power state of the device, because
		 * the same power resources may be referenced by other devices.
		 *
		 * For systems predating ACPI 4.0 we assume that D3hot is the
		 * deepest state that can be supported.
		 */
		if (psc > result && psc < ACPI_STATE_D3_COLD)
			result = psc;
		else if (result == ACPI_STATE_UNKNOWN)
			result = psc > ACPI_STATE_D2 ? ACPI_STATE_D3_HOT : psc;
	}

	/*
	 * If we were unsure about the device parent's power state up to this
	 * point, the fact that the device is in D0 implies that the parent has
	 * to be in D0 too, except if ignore_parent is set.
	 */
	if (!device->power.flags.ignore_parent && parent &&
	    parent->power.state == ACPI_STATE_UNKNOWN &&
	    result == ACPI_STATE_D0)
		parent->power.state = ACPI_STATE_D0;

	*state = result;

 out:
	acpi_handle_debug(device->handle, "Power state: %s\n",
			  acpi_power_state_string(*state));

	return 0;
}

static int acpi_dev_pm_explicit_set(struct acpi_device *adev, int state)
{
	if (adev->power.states[state].flags.explicit_set) {
		char method[5] = { '_', 'P', 'S', '0' + state, '\0' };
		acpi_status status;

		status = acpi_evaluate_object(adev->handle, method, NULL, NULL);
		if (ACPI_FAILURE(status))
			return -ENODEV;
	}
	return 0;
}

/**
 * acpi_device_set_power - Set power state of an ACPI device.
 * @device: Device to set the power state of.
 * @state: New power state to set.
 *
 * Callers must ensure that the device is power manageable before using this
 * function.
 */
int acpi_device_set_power(struct acpi_device *device, int state)
{
	int target_state = state;
	int result = 0;

	if (!device || !device->flags.power_manageable
	    || (state < ACPI_STATE_D0) || (state > ACPI_STATE_D3_COLD))
		return -EINVAL;

	acpi_handle_debug(device->handle, "Power state change: %s -> %s\n",
			  acpi_power_state_string(device->power.state),
			  acpi_power_state_string(state));

	/* Make sure this is a valid target state */

	/* There is a special case for D0 addressed below. */
	if (state > ACPI_STATE_D0 && state == device->power.state)
		goto no_change;

	if (state == ACPI_STATE_D3_COLD) {
		/*
		 * For transitions to D3cold we need to execute _PS3 and then
		 * possibly drop references to the power resources in use.
		 */
		state = ACPI_STATE_D3_HOT;
		/* If D3cold is not supported, use D3hot as the target state. */
		if (!device->power.states[ACPI_STATE_D3_COLD].flags.valid)
			target_state = state;
	} else if (!device->power.states[state].flags.valid) {
		acpi_handle_debug(device->handle, "Power state %s not supported\n",
				  acpi_power_state_string(state));
		return -ENODEV;
	}

	if (!device->power.flags.ignore_parent) {
		struct acpi_device *parent;

		parent = acpi_dev_parent(device);
		if (parent && state < parent->power.state) {
			acpi_handle_debug(device->handle,
					  "Cannot transition to %s for parent in %s\n",
					  acpi_power_state_string(state),
					  acpi_power_state_string(parent->power.state));
			return -ENODEV;
		}
	}

	/*
	 * Transition Power
	 * ----------------
	 * In accordance with ACPI 6, _PSx is executed before manipulating power
	 * resources, unless the target state is D0, in which case _PS0 is
	 * supposed to be executed after turning the power resources on.
	 */
	if (state > ACPI_STATE_D0) {
		/*
		 * According to ACPI 6, devices cannot go from lower-power
		 * (deeper) states to higher-power (shallower) states.
		 */
		if (state < device->power.state) {
			acpi_handle_debug(device->handle,
					  "Cannot transition from %s to %s\n",
					  acpi_power_state_string(device->power.state),
					  acpi_power_state_string(state));
			return -ENODEV;
		}

		/*
		 * If the device goes from D3hot to D3cold, _PS3 has been
		 * evaluated for it already, so skip it in that case.
		 */
		if (device->power.state < ACPI_STATE_D3_HOT) {
			result = acpi_dev_pm_explicit_set(device, state);
			if (result)
				goto end;
		}

		if (device->power.flags.power_resources)
			result = acpi_power_transition(device, target_state);
	} else {
		int cur_state = device->power.state;

		if (device->power.flags.power_resources) {
			result = acpi_power_transition(device, ACPI_STATE_D0);
			if (result)
				goto end;
		}

		if (cur_state == ACPI_STATE_D0) {
			int psc;

			/* Nothing to do here if _PSC is not present. */
			if (!device->power.flags.explicit_get)
				goto no_change;

			/*
			 * The power state of the device was set to D0 last
			 * time, but that might have happened before a
			 * system-wide transition involving the platform
			 * firmware, so it may be necessary to evaluate _PS0
			 * for the device here.  However, use extra care here
			 * and evaluate _PSC to check the device's current power
			 * state, and only invoke _PS0 if the evaluation of _PSC
			 * is successful and it returns a power state different
			 * from D0.
			 */
			result = acpi_dev_pm_explicit_get(device, &psc);
			if (result || psc == ACPI_STATE_D0)
				goto no_change;
		}

		result = acpi_dev_pm_explicit_set(device, ACPI_STATE_D0);
	}

end:
	if (result) {
		acpi_handle_debug(device->handle,
				  "Failed to change power state to %s\n",
				  acpi_power_state_string(target_state));
	} else {
		device->power.state = target_state;
		acpi_handle_debug(device->handle, "Power state changed to %s\n",
				  acpi_power_state_string(target_state));
	}

	return result;

no_change:
	acpi_handle_debug(device->handle, "Already in %s\n",
			  acpi_power_state_string(state));
	return 0;
}
EXPORT_SYMBOL(acpi_device_set_power);

int acpi_bus_set_power(acpi_handle handle, int state)
{
	struct acpi_device *device = acpi_fetch_acpi_dev(handle);

	if (device)
		return acpi_device_set_power(device, state);

	return -ENODEV;
}
EXPORT_SYMBOL(acpi_bus_set_power);

int acpi_bus_init_power(struct acpi_device *device)
{
	int state;
	int result;

	if (!device)
		return -EINVAL;

	device->power.state = ACPI_STATE_UNKNOWN;
	if (!acpi_device_is_present(device)) {
		device->flags.initialized = false;
		return -ENXIO;
	}

	result = acpi_device_get_power(device, &state);
	if (result)
		return result;

	if (state < ACPI_STATE_D3_COLD && device->power.flags.power_resources) {
		/* Reference count the power resources. */
		result = acpi_power_on_resources(device, state);
		if (result)
			return result;

		if (state == ACPI_STATE_D0) {
			/*
			 * If _PSC is not present and the state inferred from
			 * power resources appears to be D0, it still may be
			 * necessary to execute _PS0 at this point, because
			 * another device using the same power resources may
			 * have been put into D0 previously and that's why we
			 * see D0 here.
			 */
			result = acpi_dev_pm_explicit_set(device, state);
			if (result)
				return result;
		}
	} else if (state == ACPI_STATE_UNKNOWN) {
		/*
		 * No power resources and missing _PSC?  Cross fingers and make
		 * it D0 in hope that this is what the BIOS put the device into.
		 * [We tried to force D0 here by executing _PS0, but that broke
		 * Toshiba P870-303 in a nasty way.]
		 */
		state = ACPI_STATE_D0;
	}
	device->power.state = state;
	return 0;
}

/**
 * acpi_device_fix_up_power - Force device with missing _PSC into D0.
 * @device: Device object whose power state is to be fixed up.
 *
 * Devices without power resources and _PSC, but having _PS0 and _PS3 defined,
 * are assumed to be put into D0 by the BIOS.  However, in some cases that may
 * not be the case and this function should be used then.
 */
int acpi_device_fix_up_power(struct acpi_device *device)
{
	int ret = 0;

	if (!device->power.flags.power_resources
	    && !device->power.flags.explicit_get
	    && device->power.state == ACPI_STATE_D0)
		ret = acpi_dev_pm_explicit_set(device, ACPI_STATE_D0);

	return ret;
}
EXPORT_SYMBOL_GPL(acpi_device_fix_up_power);

static int fix_up_power_if_applicable(struct acpi_device *adev, void *not_used)
{
	if (adev->status.present && adev->status.enabled)
		acpi_device_fix_up_power(adev);

	return 0;
}

/**
 * acpi_device_fix_up_power_extended - Force device and its children into D0.
 * @adev: Parent device object whose power state is to be fixed up.
 *
 * Call acpi_device_fix_up_power() for @adev and its children so long as they
 * are reported as present and enabled.
 */
void acpi_device_fix_up_power_extended(struct acpi_device *adev)
{
	acpi_device_fix_up_power(adev);
	acpi_dev_for_each_child(adev, fix_up_power_if_applicable, NULL);
}
EXPORT_SYMBOL_GPL(acpi_device_fix_up_power_extended);

/**
 * acpi_device_fix_up_power_children - Force a device's children into D0.
 * @adev: Parent device object whose children's power state is to be fixed up.
 *
 * Call acpi_device_fix_up_power() for @adev's children so long as they
 * are reported as present and enabled.
 */
void acpi_device_fix_up_power_children(struct acpi_device *adev)
{
	acpi_dev_for_each_child(adev, fix_up_power_if_applicable, NULL);
}
EXPORT_SYMBOL_GPL(acpi_device_fix_up_power_children);

int acpi_device_update_power(struct acpi_device *device, int *state_p)
{
	int state;
	int result;

	if (device->power.state == ACPI_STATE_UNKNOWN) {
		result = acpi_bus_init_power(device);
		if (!result && state_p)
			*state_p = device->power.state;

		return result;
	}

	result = acpi_device_get_power(device, &state);
	if (result)
		return result;

	if (state == ACPI_STATE_UNKNOWN) {
		state = ACPI_STATE_D0;
		result = acpi_device_set_power(device, state);
		if (result)
			return result;
	} else {
		if (device->power.flags.power_resources) {
			/*
			 * We don't need to really switch the state, bu we need
			 * to update the power resources' reference counters.
			 */
			result = acpi_power_transition(device, state);
			if (result)
				return result;
		}
		device->power.state = state;
	}
	if (state_p)
		*state_p = state;

	return 0;
}
EXPORT_SYMBOL_GPL(acpi_device_update_power);

int acpi_bus_update_power(acpi_handle handle, int *state_p)
{
	struct acpi_device *device = acpi_fetch_acpi_dev(handle);

	if (device)
		return acpi_device_update_power(device, state_p);

	return -ENODEV;
}
EXPORT_SYMBOL_GPL(acpi_bus_update_power);

bool acpi_bus_power_manageable(acpi_handle handle)
{
	struct acpi_device *device = acpi_fetch_acpi_dev(handle);

	return device && device->flags.power_manageable;
}
EXPORT_SYMBOL(acpi_bus_power_manageable);

static int acpi_power_up_if_adr_present(struct acpi_device *adev, void *not_used)
{
	if (!(adev->flags.power_manageable && adev->pnp.type.bus_address))
		return 0;

	acpi_handle_debug(adev->handle, "Power state: %s\n",
			  acpi_power_state_string(adev->power.state));

	if (adev->power.state == ACPI_STATE_D3_COLD)
		return acpi_device_set_power(adev, ACPI_STATE_D0);

	return 0;
}

/**
 * acpi_dev_power_up_children_with_adr - Power up childres with valid _ADR
 * @adev: Parent ACPI device object.
 *
 * Change the power states of the direct children of @adev that are in D3cold
 * and hold valid _ADR objects to D0 in order to allow bus (e.g. PCI)
 * enumeration code to access them.
 */
void acpi_dev_power_up_children_with_adr(struct acpi_device *adev)
{
	acpi_dev_for_each_child(adev, acpi_power_up_if_adr_present, NULL);
}

/**
 * acpi_dev_power_state_for_wake - Deepest power state for wakeup signaling
 * @adev: ACPI companion of the target device.
 *
 * Evaluate _S0W for @adev and return the value produced by it or return
 * ACPI_STATE_UNKNOWN on errors (including _S0W not present).
 */
u8 acpi_dev_power_state_for_wake(struct acpi_device *adev)
{
	unsigned long long state;
	acpi_status status;

	status = acpi_evaluate_integer(adev->handle, "_S0W", NULL, &state);
	if (ACPI_FAILURE(status))
		return ACPI_STATE_UNKNOWN;

	return state;
}
