/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_X86_SHSTK_H
#define _ASM_X86_SHSTK_H

#ifndef __ASSEMBLER__
#include <linux/types.h>

struct task_struct;
struct ksignal;

static inline long shstk_prctl(struct task_struct *task, int option,
			       unsigned long arg2) { return -EINVAL; }
static inline void reset_thread_features(void) {}
static inline unsigned long shstk_alloc_thread_stack(struct task_struct *p,
						     u64 clone_flags,
						     unsigned long stack_size) { return 0; }
static inline void shstk_free(struct task_struct *p) {}
static inline int setup_signal_shadow_stack(struct ksignal *ksig) { return 0; }
static inline int restore_signal_shadow_stack(void) { return 0; }
static inline int shstk_update_last_frame(unsigned long val) { return 0; }
static inline bool shstk_is_enabled(void) { return false; }
static inline int shstk_pop(u64 *val) { return -ENOTSUPP; }
static inline int shstk_push(u64 val) { return -ENOTSUPP; }

#endif /* __ASSEMBLER__ */

#endif /* _ASM_X86_SHSTK_H */
