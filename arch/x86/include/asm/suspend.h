/* SPDX-License-Identifier: GPL-2.0 */
# include <asm/suspend_64.h>
extern unsigned long restore_jump_address __visible;
extern unsigned long jump_address_phys;
extern unsigned long restore_cr3 __visible;
extern unsigned long temp_pgt __visible;
extern unsigned long relocated_restore_code __visible;
extern int relocate_restore_code(void);
/* Defined in hibernate_asm_32/64.S */
extern asmlinkage __visible int restore_image(void);
