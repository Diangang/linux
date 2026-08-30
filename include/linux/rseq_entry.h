/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_RSEQ_ENTRY_H
#define _LINUX_RSEQ_ENTRY_H

/* Must be outside the CONFIG_RSEQ guard to resolve the stubs */
#define rseq_stat_inc(x)	do { } while (0)

static inline void rseq_note_user_irq_entry(void) { }
static inline bool rseq_exit_to_user_mode_restart(struct pt_regs *regs, unsigned long ti_work)
{
	return false;
}
static inline void rseq_syscall_exit_to_user_mode(void) { }
static inline void rseq_irqentry_exit_to_user_mode(void) { }
static inline void rseq_exit_to_user_mode_legacy(void) { }
static inline void rseq_debug_syscall_return(struct pt_regs *regs) { }
static inline bool rseq_grant_slice_extension(unsigned long ti_work, unsigned long mask) { return false; }

#endif /* _LINUX_RSEQ_ENTRY_H */
