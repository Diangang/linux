// SPDX-License-Identifier: GPL-2.0
/*
 *  syscore.c - Execution of system core operations.
 *
 *  Copyright (C) 2011 Rafael J. Wysocki <rjw@sisk.pl>, Novell Inc.
 */

#include <linux/syscore_ops.h>
#include <linux/mutex.h>
#include <linux/module.h>
#include <linux/suspend.h>

static LIST_HEAD(syscore_list);
static DEFINE_MUTEX(syscore_lock);

/**
 * register_syscore - Register a set of system core operations.
 * @syscore: System core operations to register.
 */
void register_syscore(struct syscore *syscore)
{
	mutex_lock(&syscore_lock);
	list_add_tail(&syscore->node, &syscore_list);
	mutex_unlock(&syscore_lock);
}
EXPORT_SYMBOL_GPL(register_syscore);

/**
 * unregister_syscore - Unregister a set of system core operations.
 * @syscore: System core operations to unregister.
 */
void unregister_syscore(struct syscore *syscore)
{
	mutex_lock(&syscore_lock);
	list_del(&syscore->node);
	mutex_unlock(&syscore_lock);
}
EXPORT_SYMBOL_GPL(unregister_syscore);


/**
 * syscore_shutdown - Execute all the registered system core shutdown callbacks.
 */
void syscore_shutdown(void)
{
	struct syscore *syscore;

	mutex_lock(&syscore_lock);

	list_for_each_entry_reverse(syscore, &syscore_list, node)
		if (syscore->ops->shutdown) {
			if (initcall_debug)
				pr_info("PM: Calling %pS\n",
					syscore->ops->shutdown);
			syscore->ops->shutdown(syscore->data);
		}

	mutex_unlock(&syscore_lock);
}
