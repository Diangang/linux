/* SPDX-License-Identifier: GPL-2.0 */
/*
 * NOTE:
 *
 * This header has combined a lot of unrelated to each other stuff.
 * The process of splitting its content is in progress while keeping
 * backward compatibility. That's why it's highly recommended NOT to
 * include this header inside another header file, especially under
 * generic or architectural include/ directory.
 */
#ifndef _LINUX_KERNEL_H
#define _LINUX_KERNEL_H

#include <linux/stdarg.h>
#include <linux/align.h>
#include <linux/array_size.h>
#include <linux/limits.h>
#include <linux/linkage.h>
#include <linux/stddef.h>
#include <linux/types.h>
#include <linux/compiler.h>
#include <linux/container_of.h>
#include <linux/bitops.h>
#include <linux/kstrtox.h>
#include <linux/log2.h>
#include <linux/math.h>
#include <linux/minmax.h>
#include <linux/typecheck.h>
#include <linux/panic.h>
#include <linux/printk.h>
#include <linux/build_bug.h>
#include <linux/sprintf.h>
#include <linux/static_call_types.h>
#include <linux/trace_printk.h>
#include <linux/util_macros.h>
#include <linux/wordpart.h>

#include <asm/byteorder.h>

#include <uapi/linux/kernel.h>

struct completion;
struct user;

#ifdef CONFIG_PREEMPT_VOLUNTARY_BUILD

extern int __cond_resched(void);
# define might_resched() __cond_resched()

#elif defined(CONFIG_PREEMPT_DYNAMIC) && defined(CONFIG_HAVE_PREEMPT_DYNAMIC_CALL)

extern int __cond_resched(void);

DECLARE_STATIC_CALL(might_resched, __cond_resched);

static __always_inline void might_resched(void)
{
	static_call_mod(might_resched)();
}

#elif defined(CONFIG_PREEMPT_DYNAMIC) && defined(CONFIG_HAVE_PREEMPT_DYNAMIC_KEY)

extern int dynamic_might_resched(void);
# define might_resched() dynamic_might_resched()

#else

# define might_resched() do { } while (0)

#endif /* CONFIG_PREEMPT_* */

  static inline void __might_resched(const char *file, int line,
				     unsigned int offsets) { }
static inline void __might_sleep(const char *file, int line) { }
# define might_sleep() do { might_resched(); } while (0)
# define cant_sleep() do { } while (0)
# define cant_migrate()		do { } while (0)
# define sched_annotate_sleep() do { } while (0)
# define non_block_start() do { } while (0)
# define non_block_end() do { } while (0)

#define might_sleep_if(cond) do { if (cond) might_sleep(); } while (0)

#if defined(CONFIG_MMU) && \
	(defined(CONFIG_PROVE_LOCKING) || defined(CONFIG_DEBUG_ATOMIC_SLEEP))
#define might_fault() __might_fault(__FILE__, __LINE__)
void __might_fault(const char *file, int line);
#else
static inline void might_fault(void) { }
#endif

void do_exit(long error_code) __noreturn;

extern int core_kernel_text(unsigned long addr);
extern int __kernel_text_address(unsigned long addr);
extern int kernel_text_address(unsigned long addr);
extern int func_ptr_is_kernel_text(void *ptr);

extern void bust_spinlocks(int yes);

extern int root_mountflags;

extern bool early_boot_irqs_disabled;

/**
 * enum system_states - Values used for system_state.
 *
 * @SYSTEM_BOOTING:	%0, no init needed
 * @SYSTEM_SCHEDULING: system is ready for scheduling; OK to use RCU
 * @SYSTEM_FREEING_INITMEM: system is freeing all of initmem; almost running
 * @SYSTEM_RUNNING:	system is up and running
 * @SYSTEM_HALT:	system entered clean system halt state
 * @SYSTEM_POWER_OFF:	system entered shutdown/clean power off state
 * @SYSTEM_RESTART:	system entered emergency power off or normal restart
 * @SYSTEM_SUSPEND:	system entered suspend or hibernate state
 *
 * Note:
 * Ordering of the states must not be changed
 * as code checks for <, <=, >, >= STATE.
 */
enum system_states {
	SYSTEM_BOOTING,
	SYSTEM_SCHEDULING,
	SYSTEM_FREEING_INITMEM,
	SYSTEM_RUNNING,
	SYSTEM_HALT,
	SYSTEM_POWER_OFF,
	SYSTEM_RESTART,
	SYSTEM_SUSPEND,
};
extern enum system_states system_state;

#endif
