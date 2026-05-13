/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_USER_RETURN_NOTIFIER_H
#define _LINUX_USER_RETURN_NOTIFIER_H


struct user_return_notifier {};

static inline void propagate_user_return_notify(struct task_struct *prev,
						struct task_struct *next)
{
}

static inline void fire_user_return_notifiers(void) {}

static inline void clear_user_return_notifier(struct task_struct *p) {}


#endif
