/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __ASM_GENERIC_MMIOWB_H
#define __ASM_GENERIC_MMIOWB_H

/*
 * Generic implementation of mmiowb() tracking for spinlocks.
 *
 * If your architecture doesn't ensure that writes to an I/O peripheral
 * within two spinlocked sections on two different CPUs are seen by the
 * peripheral in the order corresponding to the lock handover, then you
 * need to follow these FIVE easy steps:
 *
 * 	1. Implement mmiowb() (and arch_mmiowb_state() if you're fancy)
 *	   in asm/mmiowb.h, then #include this file
 *	2. Ensure your I/O write accessors call mmiowb_set_pending()
 *	3. Untangle the resulting mess of header files
 *	4. Complain to your architects
 */
#define mmiowb_set_pending()		do { } while (0)
#define mmiowb_spin_lock()		do { } while (0)
#define mmiowb_spin_unlock()		do { } while (0)
#endif	/* __ASM_GENERIC_MMIOWB_H */
