// SPDX-License-Identifier: GPL-2.0-only
/* By Ross Biro 1/23/92 */
/*
 * Pentium III FXSR, SSE support
 *	Gareth Hughes <gareth@valinux.com>, May 2000
 */

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/sched/task_stack.h>
#include <linux/mm.h>
#include <linux/smp.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/ptrace.h>
#include <linux/user.h>
#include <linux/elf.h>
#include <linux/security.h>
#include <linux/audit.h>
#include <linux/seccomp.h>
#include <linux/signal.h>
#include <linux/rcupdate.h>
#include <linux/export.h>
#include <linux/context_tracking.h>
#include <linux/nospec.h>

#include <linux/uaccess.h>
#include <asm/processor.h>
#include <asm/fpu/signal.h>
#include <asm/fpu/regset.h>
#include <asm/fpu/xstate.h>
#include <asm/debugreg.h>
#include <asm/ldt.h>
#include <asm/desc.h>
#include <asm/prctl.h>
#include <asm/proto.h>
#include <asm/traps.h>
#include <asm/syscall.h>
#include <asm/fsgsbase.h>
#include <asm/io_bitmap.h>

#include "tls.h"

enum x86_regset_32 {
	REGSET32_GENERAL,
	REGSET32_FP,
	REGSET32_XFP,
	REGSET32_XSTATE,
	REGSET32_TLS,
	REGSET32_IOPERM,
};

enum x86_regset_64 {
	REGSET64_GENERAL,
	REGSET64_FP,
	REGSET64_IOPERM,
	REGSET64_XSTATE,
	REGSET64_SSP,
};

#define REGSET_GENERAL \
({ \
	BUILD_BUG_ON((int)REGSET32_GENERAL != (int)REGSET64_GENERAL); \
	REGSET32_GENERAL; \
})

#define REGSET_FP \
({ \
	BUILD_BUG_ON((int)REGSET32_FP != (int)REGSET64_FP); \
	REGSET32_FP; \
})


struct pt_regs_offset {
	const char *name;
	int offset;
};

#define REG_OFFSET_NAME(r) {.name = #r, .offset = offsetof(struct pt_regs, r)}
#define REG_OFFSET_END {.name = NULL, .offset = 0}

static const struct pt_regs_offset regoffset_table[] = {
#ifdef CONFIG_X86_64
	REG_OFFSET_NAME(r15),
	REG_OFFSET_NAME(r14),
	REG_OFFSET_NAME(r13),
	REG_OFFSET_NAME(r12),
	REG_OFFSET_NAME(r11),
	REG_OFFSET_NAME(r10),
	REG_OFFSET_NAME(r9),
	REG_OFFSET_NAME(r8),
#endif
	REG_OFFSET_NAME(bx),
	REG_OFFSET_NAME(cx),
	REG_OFFSET_NAME(dx),
	REG_OFFSET_NAME(si),
	REG_OFFSET_NAME(di),
	REG_OFFSET_NAME(bp),
	REG_OFFSET_NAME(ax),
	REG_OFFSET_NAME(orig_ax),
	REG_OFFSET_NAME(ip),
	REG_OFFSET_NAME(cs),
	REG_OFFSET_NAME(flags),
	REG_OFFSET_NAME(sp),
	REG_OFFSET_NAME(ss),
	REG_OFFSET_END,
};

/**
 * regs_query_register_offset() - query register offset from its name
 * @name:	the name of a register
 *
 * regs_query_register_offset() returns the offset of a register in struct
 * pt_regs from its name. If the name is invalid, this returns -EINVAL;
 */
int regs_query_register_offset(const char *name)
{
	const struct pt_regs_offset *roff;
	for (roff = regoffset_table; roff->name != NULL; roff++)
		if (!strcmp(roff->name, name))
			return roff->offset;
	return -EINVAL;
}

/**
 * regs_query_register_name() - query register name from its offset
 * @offset:	the offset of a register in struct pt_regs.
 *
 * regs_query_register_name() returns the name of a register from its
 * offset in struct pt_regs. If the @offset is invalid, this returns NULL;
 */
const char *regs_query_register_name(unsigned int offset)
{
	const struct pt_regs_offset *roff;
	for (roff = regoffset_table; roff->name != NULL; roff++)
		if (roff->offset == offset)
			return roff->name;
	return NULL;
}

/*
 * does not yet catch signals sent when the child dies.
 * in exit.c or in signal.c.
 */

/*
 * Determines which flags the user has access to [1 = access, 0 = no access].
 */
#define FLAG_MASK_32		((unsigned long)			\
				 (X86_EFLAGS_CF | X86_EFLAGS_PF |	\
				  X86_EFLAGS_AF | X86_EFLAGS_ZF |	\
				  X86_EFLAGS_SF | X86_EFLAGS_TF |	\
				  X86_EFLAGS_DF | X86_EFLAGS_OF |	\
				  X86_EFLAGS_RF | X86_EFLAGS_AC))

/*
 * Determines whether a value may be installed in a segment register.
 */
static inline bool invalid_selector(u16 value)
{
	return unlikely(value != 0 && (value & SEGMENT_RPL_MASK) != USER_RPL);
}


#define FLAG_MASK		(FLAG_MASK_32 | X86_EFLAGS_NT)

static unsigned long *pt_regs_access(struct pt_regs *regs, unsigned long offset)
{
	BUILD_BUG_ON(offsetof(struct pt_regs, r15) != 0);
	return &regs->r15 + (offset / sizeof(regs->r15));
}

static u16 get_segment_reg(struct task_struct *task, unsigned long offset)
{
	/*
	 * Returning the value truncates it to 16 bits.
	 */
	unsigned int seg;

	switch (offset) {
	case offsetof(struct user_regs_struct, fs):
		if (task == current) {
			/* Older gas can't assemble movq %?s,%r?? */
			asm("movl %%fs,%0" : "=r" (seg));
			return seg;
		}
		return task->thread.fsindex;
	case offsetof(struct user_regs_struct, gs):
		if (task == current) {
			asm("movl %%gs,%0" : "=r" (seg));
			return seg;
		}
		return task->thread.gsindex;
	case offsetof(struct user_regs_struct, ds):
		if (task == current) {
			asm("movl %%ds,%0" : "=r" (seg));
			return seg;
		}
		return task->thread.ds;
	case offsetof(struct user_regs_struct, es):
		if (task == current) {
			asm("movl %%es,%0" : "=r" (seg));
			return seg;
		}
		return task->thread.es;

	case offsetof(struct user_regs_struct, cs):
	case offsetof(struct user_regs_struct, ss):
		break;
	}
	return *pt_regs_access(task_pt_regs(task), offset);
}

static int set_segment_reg(struct task_struct *task,
			   unsigned long offset, u16 value)
{
	if (WARN_ON_ONCE(task == current))
		return -EIO;

	/*
	 * The value argument was already truncated to 16 bits.
	 */
	if (invalid_selector(value))
		return -EIO;

	/*
	 * Writes to FS and GS will change the stored selector.  Whether
	 * this changes the segment base as well depends on whether
	 * FSGSBASE is enabled.
	 */

	switch (offset) {
	case offsetof(struct user_regs_struct,fs):
		task->thread.fsindex = value;
		break;
	case offsetof(struct user_regs_struct,gs):
		task->thread.gsindex = value;
		break;
	case offsetof(struct user_regs_struct,ds):
		task->thread.ds = value;
		break;
	case offsetof(struct user_regs_struct,es):
		task->thread.es = value;
		break;

		/*
		 * Can't actually change these in 64-bit mode.
		 */
	case offsetof(struct user_regs_struct,cs):
		if (unlikely(value == 0))
			return -EIO;
		task_pt_regs(task)->cs = value;
		break;
	case offsetof(struct user_regs_struct,ss):
		if (unlikely(value == 0))
			return -EIO;
		task_pt_regs(task)->ss = value;
		break;
	}

	return 0;
}


static unsigned long get_flags(struct task_struct *task)
{
	unsigned long retval = task_pt_regs(task)->flags;

	/*
	 * If the debugger set TF, hide it from the readout.
	 */
	if (test_tsk_thread_flag(task, TIF_FORCED_TF))
		retval &= ~X86_EFLAGS_TF;

	return retval;
}

static int set_flags(struct task_struct *task, unsigned long value)
{
	struct pt_regs *regs = task_pt_regs(task);

	/*
	 * If the user value contains TF, mark that
	 * it was not "us" (the debugger) that set it.
	 * If not, make sure it stays set if we had.
	 */
	if (value & X86_EFLAGS_TF)
		clear_tsk_thread_flag(task, TIF_FORCED_TF);
	else if (test_tsk_thread_flag(task, TIF_FORCED_TF))
		value |= X86_EFLAGS_TF;

	regs->flags = (regs->flags & ~FLAG_MASK) | (value & FLAG_MASK);

	return 0;
}

static int putreg(struct task_struct *child,
		  unsigned long offset, unsigned long value)
{
	switch (offset) {
	case offsetof(struct user_regs_struct, cs):
	case offsetof(struct user_regs_struct, ds):
	case offsetof(struct user_regs_struct, es):
	case offsetof(struct user_regs_struct, fs):
	case offsetof(struct user_regs_struct, gs):
	case offsetof(struct user_regs_struct, ss):
		return set_segment_reg(child, offset, value);

	case offsetof(struct user_regs_struct, flags):
		return set_flags(child, value);

#ifdef CONFIG_X86_64
	case offsetof(struct user_regs_struct,fs_base):
		if (value >= TASK_SIZE_MAX)
			return -EIO;
		x86_fsbase_write_task(child, value);
		return 0;
	case offsetof(struct user_regs_struct,gs_base):
		if (value >= TASK_SIZE_MAX)
			return -EIO;
		x86_gsbase_write_task(child, value);
		return 0;
#endif
	}

	*pt_regs_access(task_pt_regs(child), offset) = value;
	return 0;
}

static unsigned long getreg(struct task_struct *task, unsigned long offset)
{
	switch (offset) {
	case offsetof(struct user_regs_struct, cs):
	case offsetof(struct user_regs_struct, ds):
	case offsetof(struct user_regs_struct, es):
	case offsetof(struct user_regs_struct, fs):
	case offsetof(struct user_regs_struct, gs):
	case offsetof(struct user_regs_struct, ss):
		return get_segment_reg(task, offset);

	case offsetof(struct user_regs_struct, flags):
		return get_flags(task);

#ifdef CONFIG_X86_64
	case offsetof(struct user_regs_struct, fs_base):
		return x86_fsbase_read_task(task);
	case offsetof(struct user_regs_struct, gs_base):
		return x86_gsbase_read_task(task);
#endif
	}

	return *pt_regs_access(task_pt_regs(task), offset);
}

static int genregs_get(struct task_struct *target,
		       const struct user_regset *regset,
		       struct membuf to)
{
	int reg;

	for (reg = 0; to.left; reg++)
		membuf_store(&to, getreg(target, reg * sizeof(unsigned long)));
	return 0;
}

static int genregs_set(struct task_struct *target,
		       const struct user_regset *regset,
		       unsigned int pos, unsigned int count,
		       const void *kbuf, const void __user *ubuf)
{
	int ret = 0;
	if (kbuf) {
		const unsigned long *k = kbuf;
		while (count >= sizeof(*k) && !ret) {
			ret = putreg(target, pos, *k++);
			count -= sizeof(*k);
			pos += sizeof(*k);
		}
	} else {
		const unsigned long  __user *u = ubuf;
		while (count >= sizeof(*u) && !ret) {
			unsigned long word;
			ret = __get_user(word, u++);
			if (ret)
				break;
			ret = putreg(target, pos, word);
			count -= sizeof(*u);
			pos += sizeof(*u);
		}
	}
	return ret;
}

/*
 * Handle PTRACE_PEEKUSR calls for the debug register area.
 */
static unsigned long ptrace_get_debugreg(struct task_struct *tsk, int n)
{
	struct thread_struct *thread = &tsk->thread;
	unsigned long val = 0;

	if (n < HBP_NUM) {
		val = 0;
	} else if (n == 6) {
		val = thread->virtual_dr6 ^ DR6_RESERVED; /* Flip back to arch polarity */
	} else if (n == 7) {
		val = thread->ptrace_dr7;
	}
	return val;
}

/*
 * Handle PTRACE_POKEUSR calls for the debug register area.
 */
static int ptrace_set_debugreg(struct task_struct *tsk, int n,
			       unsigned long val)
{
	struct thread_struct *thread = &tsk->thread;
	/* There are no DR4 or DR5 registers */
	int rc = -EIO;

	if (n < HBP_NUM) {
		rc = -EIO;
	} else if (n == 6) {
		thread->virtual_dr6 = val ^ DR6_RESERVED; /* Flip to positive polarity */
		rc = 0;
	} else if (n == 7) {
		rc = -EIO;
	}
	return rc;
}

/*
 * These access the current or another (stopped) task's io permission
 * bitmap for debugging or core dump.
 */
static int ioperm_active(struct task_struct *target,
			 const struct user_regset *regset)
{
	struct io_bitmap *iobm = target->thread.io_bitmap;

	return iobm ? DIV_ROUND_UP(iobm->max, regset->size) : 0;
}

static int ioperm_get(struct task_struct *target,
		      const struct user_regset *regset,
		      struct membuf to)
{
	struct io_bitmap *iobm = target->thread.io_bitmap;

	if (!iobm)
		return -ENXIO;

	return membuf_write(&to, iobm->bitmap, IO_BITMAP_BYTES);
}

/*
 * Called by kernel/ptrace.c when detaching..
 *
 * Make sure the single step bit is not set.
 */
void ptrace_disable(struct task_struct *child)
{
	user_disable_single_step(child);
}

#ifdef CONFIG_X86_64
static const struct user_regset_view user_x86_64_view; /* Initialized below. */
#endif

long arch_ptrace(struct task_struct *child, long request,
		 unsigned long addr, unsigned long data)
{
	int ret;
	unsigned long __user *datap = (unsigned long __user *)data;

#ifdef CONFIG_X86_64
	/* This is native 64-bit ptrace() */
	const struct user_regset_view *regset_view = &user_x86_64_view;
#else
	/* This is native 32-bit ptrace() */
	const struct user_regset_view *regset_view = &user_x86_32_view;
#endif

	switch (request) {
	/* read the word at location addr in the USER area. */
	case PTRACE_PEEKUSR: {
		unsigned long tmp;

		ret = -EIO;
		if ((addr & (sizeof(data) - 1)) || addr >= sizeof(struct user))
			break;

		tmp = 0;  /* Default return condition */
		if (addr < sizeof(struct user_regs_struct))
			tmp = getreg(child, addr);
		else if (addr >= offsetof(struct user, u_debugreg[0]) &&
			 addr <= offsetof(struct user, u_debugreg[7])) {
			addr -= offsetof(struct user, u_debugreg[0]);
			tmp = ptrace_get_debugreg(child, addr / sizeof(data));
		}
		ret = put_user(tmp, datap);
		break;
	}

	case PTRACE_POKEUSR: /* write the word at location addr in the USER area */
		ret = -EIO;
		if ((addr & (sizeof(data) - 1)) || addr >= sizeof(struct user))
			break;

		if (addr < sizeof(struct user_regs_struct))
			ret = putreg(child, addr, data);
		else if (addr >= offsetof(struct user, u_debugreg[0]) &&
			 addr <= offsetof(struct user, u_debugreg[7])) {
			addr -= offsetof(struct user, u_debugreg[0]);
			ret = ptrace_set_debugreg(child,
						  addr / sizeof(data), data);
		}
		break;

	case PTRACE_GETREGS:	/* Get all gp regs from the child. */
		return copy_regset_to_user(child,
					   regset_view,
					   REGSET_GENERAL,
					   0, sizeof(struct user_regs_struct),
					   datap);

	case PTRACE_SETREGS:	/* Set all gp regs in the child. */
		return copy_regset_from_user(child,
					     regset_view,
					     REGSET_GENERAL,
					     0, sizeof(struct user_regs_struct),
					     datap);

	case PTRACE_GETFPREGS:	/* Get the child FPU state. */
		return copy_regset_to_user(child,
					   regset_view,
					   REGSET_FP,
					   0, sizeof(struct user_i387_struct),
					   datap);

	case PTRACE_SETFPREGS:	/* Set the child FPU state. */
		return copy_regset_from_user(child,
					     regset_view,
					     REGSET_FP,
					     0, sizeof(struct user_i387_struct),
					     datap);



#ifdef CONFIG_X86_64
		/* normal 64bit interface to access TLS data.
		   Works just like arch_prctl, except that the arguments
		   are reversed. */
	case PTRACE_ARCH_PRCTL:
		ret = do_arch_prctl_64(child, data, addr);
		break;
#endif

	default:
		ret = ptrace_request(child, request, addr, data);
		break;
	}

	return ret;
}




#ifdef CONFIG_X86_64

static struct user_regset x86_64_regsets[] __ro_after_init = {
	[REGSET64_GENERAL] = {
		USER_REGSET_NOTE_TYPE(PRSTATUS),
		.n		= sizeof(struct user_regs_struct) / sizeof(long),
		.size		= sizeof(long),
		.align		= sizeof(long),
		.regset_get	= genregs_get,
		.set		= genregs_set
	},
	[REGSET64_FP] = {
		USER_REGSET_NOTE_TYPE(PRFPREG),
		.n		= sizeof(struct fxregs_state) / sizeof(long),
		.size		= sizeof(long),
		.align		= sizeof(long),
		.active		= regset_xregset_fpregs_active,
		.regset_get	= xfpregs_get,
		.set		= xfpregs_set
	},
	[REGSET64_XSTATE] = {
		USER_REGSET_NOTE_TYPE(X86_XSTATE),
		.size		= sizeof(u64),
		.align		= sizeof(u64),
		.active		= xstateregs_active,
		.regset_get	= xstateregs_get,
		.set		= xstateregs_set
	},
	[REGSET64_IOPERM] = {
		USER_REGSET_NOTE_TYPE(386_IOPERM),
		.n		= IO_BITMAP_LONGS,
		.size		= sizeof(long),
		.align		= sizeof(long),
		.active		= ioperm_active,
		.regset_get	= ioperm_get
	},
};

static const struct user_regset_view user_x86_64_view = {
	.name = "x86_64", .e_machine = EM_X86_64,
	.regsets = x86_64_regsets, .n = ARRAY_SIZE(x86_64_regsets)
};

#else  /* CONFIG_X86_32 */

#define user_regs_struct32	user_regs_struct
#define genregs32_get		genregs_get
#define genregs32_set		genregs_set

#endif	/* CONFIG_X86_64 */


/*
 * This represents bytes 464..511 in the memory layout exported through
 * the REGSET_XSTATE interface.
 */
u64 xstate_fx_sw_bytes[USER_XSTATE_FX_SW_WORDS];

void __init update_regset_xstate_info(unsigned int size, u64 xstate_mask)
{
#ifdef CONFIG_X86_64
	x86_64_regsets[REGSET64_XSTATE].n = size / sizeof(u64);
#endif
	xstate_fx_sw_bytes[USER_XSTATE_XCR0_WORD] = xstate_mask;
}

/*
 * This is used by the core dump code to decide which regset to dump.  The
 * core dump code writes out the resulting .e_machine and the corresponding
 * regsets.  This is suboptimal if the task is messing around with its CS.L
 * field, but at worst the core dump will end up missing some information.
 *
 * Unfortunately, it is also used by the broken PTRACE_GETREGSET and
 * PTRACE_SETREGSET APIs.  These APIs look at the .regsets field but have
 * no way to make sure that the e_machine they use matches the caller's
 * expectations.  The result is that the data format returned by
 * PTRACE_GETREGSET depends on the returned CS field (and even the offset
 * of the returned CS field depends on its value!) and the data format
 * accepted by PTRACE_SETREGSET is determined by the old CS value.  The
 * upshot is that it is basically impossible to use these APIs correctly.
 *
 * The best way to fix it in the long run would probably be to add new
 * improved ptrace() APIs to read and write registers reliably, possibly by
 * allowing userspace to select the ELF e_machine variant that they expect.
 */
const struct user_regset_view *task_user_regset_view(struct task_struct *task)
{
#ifdef CONFIG_X86_64
	return &user_x86_64_view;
#endif
}

void send_sigtrap(struct pt_regs *regs, int error_code, int si_code)
{
	struct task_struct *tsk = current;

	tsk->thread.trap_nr = X86_TRAP_DB;
	tsk->thread.error_code = error_code;

	/* Send us the fake SIGTRAP */
	force_sig_fault(SIGTRAP, si_code,
			user_mode(regs) ? (void __user *)regs->ip : NULL);
}

void user_single_step_report(struct pt_regs *regs)
{
	send_sigtrap(regs, 0, TRAP_BRKPT);
}
