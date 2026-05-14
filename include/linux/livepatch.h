/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef _LINUX_LIVEPATCH_H_
#define _LINUX_LIVEPATCH_H_

struct task_struct;

static inline bool klp_patch_pending(struct task_struct *task)
{
	return false;
}

static inline void klp_update_patch_state(struct task_struct *task)
{
}

static inline void klp_copy_process(struct task_struct *child)
{
}

#endif /* _LINUX_LIVEPATCH_H_ */
