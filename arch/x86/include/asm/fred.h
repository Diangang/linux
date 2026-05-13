/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Macros for Flexible Return and Event Delivery (FRED)
 */

#ifndef ASM_X86_FRED_H
#define ASM_X86_FRED_H

#include <linux/const.h>

#include <asm/asm.h>
#include <asm/msr.h>
#include <asm/trapnr.h>

/*
 * FRED event return instruction opcodes for ERET{S,U}; supported in
 * binutils >= 2.41.
 */
#define ERETS			_ASM_BYTES(0xf2,0x0f,0x01,0xca)
#define ERETU			_ASM_BYTES(0xf3,0x0f,0x01,0xca)

/*
 * RSP is aligned to a 64-byte boundary before used to push a new stack frame
 */
#define FRED_STACK_FRAME_RSP_MASK	_AT(unsigned long, (~0x3f))

/*
 * Used for the return address for call emulation during code patching,
 * and measured in 64-byte cache lines.
 */
#define FRED_CONFIG_REDZONE_AMOUNT	1
#define FRED_CONFIG_REDZONE		(_AT(unsigned long, FRED_CONFIG_REDZONE_AMOUNT) << 6)
#define FRED_CONFIG_INT_STKLVL(l)	(_AT(unsigned long, l) << 9)
#define FRED_CONFIG_ENTRYPOINT(p)	_AT(unsigned long, (p))

#ifndef __ASSEMBLER__

static __always_inline unsigned long fred_event_data(struct pt_regs *regs) { return 0; }
static inline void cpu_init_fred_exceptions(void) { }
static inline void cpu_init_fred_rsps(void) { }
static inline void fred_complete_exception_setup(void) { }
static inline void fred_entry_from_kvm(unsigned int type, unsigned int vector) { }
static inline void fred_sync_rsp0(unsigned long rsp0) { }
static inline void fred_update_rsp0(void) { }
#endif /* !__ASSEMBLER__ */

#endif /* ASM_X86_FRED_H */
