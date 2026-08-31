// SPDX-License-Identifier: GPL-2.0-only
/*
 * Based on arch/arm/kernel/ptrace.c
 *
 * By Ross Biro 1/23/92
 * edited by Linus Torvalds
 * ARM modifications Copyright (C) 2000 Russell King
 * Copyright (C) 2012 ARM Ltd.
 */

#include <linux/compat.h>
#include <linux/kernel.h>
#include <linux/sched/signal.h>
#include <linux/sched/task_stack.h>
#include <linux/mm.h>
#include <linux/nospec.h>
#include <linux/smp.h>
#include <linux/ptrace.h>
#include <linux/user.h>
#include <linux/seccomp.h>
#include <linux/security.h>
#include <linux/init.h>
#include <linux/signal.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/regset.h>
#include <linux/elf.h>
#include <linux/rseq.h>

#include <asm/compat.h>
#include <asm/cpufeature.h>
#include <asm/debug-monitors.h>
#include <asm/fpsimd.h>
#include <asm/gcs.h>
#include <asm/mte.h>
#include <asm/stacktrace.h>
#include <asm/syscall.h>
#include <asm/traps.h>
#include <asm/system_misc.h>

struct pt_regs_offset {
	const char *name;
	int offset;
};

#define REG_OFFSET_NAME(r) {.name = #r, .offset = offsetof(struct pt_regs, r)}
#define REG_OFFSET_END {.name = NULL, .offset = 0}
#define GPR_OFFSET_NAME(r) \
	{.name = "x" #r, .offset = offsetof(struct pt_regs, regs[r])}

static const struct pt_regs_offset regoffset_table[] = {
	GPR_OFFSET_NAME(0),
	GPR_OFFSET_NAME(1),
	GPR_OFFSET_NAME(2),
	GPR_OFFSET_NAME(3),
	GPR_OFFSET_NAME(4),
	GPR_OFFSET_NAME(5),
	GPR_OFFSET_NAME(6),
	GPR_OFFSET_NAME(7),
	GPR_OFFSET_NAME(8),
	GPR_OFFSET_NAME(9),
	GPR_OFFSET_NAME(10),
	GPR_OFFSET_NAME(11),
	GPR_OFFSET_NAME(12),
	GPR_OFFSET_NAME(13),
	GPR_OFFSET_NAME(14),
	GPR_OFFSET_NAME(15),
	GPR_OFFSET_NAME(16),
	GPR_OFFSET_NAME(17),
	GPR_OFFSET_NAME(18),
	GPR_OFFSET_NAME(19),
	GPR_OFFSET_NAME(20),
	GPR_OFFSET_NAME(21),
	GPR_OFFSET_NAME(22),
	GPR_OFFSET_NAME(23),
	GPR_OFFSET_NAME(24),
	GPR_OFFSET_NAME(25),
	GPR_OFFSET_NAME(26),
	GPR_OFFSET_NAME(27),
	GPR_OFFSET_NAME(28),
	GPR_OFFSET_NAME(29),
	GPR_OFFSET_NAME(30),
	{.name = "lr", .offset = offsetof(struct pt_regs, regs[30])},
	REG_OFFSET_NAME(sp),
	REG_OFFSET_NAME(pc),
	REG_OFFSET_NAME(pstate),
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
 * regs_within_kernel_stack() - check the address in the stack
 * @regs:      pt_regs which contains kernel stack pointer.
 * @addr:      address which is checked.
 *
 * regs_within_kernel_stack() checks @addr is within the kernel stack page(s).
 * If @addr is within the kernel stack, it returns true. If not, returns false.
 */
static bool regs_within_kernel_stack(struct pt_regs *regs, unsigned long addr)
{
	return ((addr & ~(THREAD_SIZE - 1))  ==
		(kernel_stack_pointer(regs) & ~(THREAD_SIZE - 1))) ||
		on_irq_stack(addr, sizeof(unsigned long));
}

/**
 * regs_get_kernel_stack_nth() - get Nth entry of the stack
 * @regs:	pt_regs which contains kernel stack pointer.
 * @n:		stack entry number.
 *
 * regs_get_kernel_stack_nth() returns @n th entry of the kernel stack which
 * is specified by @regs. If the @n th entry is NOT in the kernel stack,
 * this returns 0.
 */
unsigned long regs_get_kernel_stack_nth(struct pt_regs *regs, unsigned int n)
{
	unsigned long *addr = (unsigned long *)kernel_stack_pointer(regs);

	addr += n;
	if (regs_within_kernel_stack(regs, (unsigned long)addr))
		return READ_ONCE_NOCHECK(*addr);
	else
		return 0;
}

/*
 * TODO: does not yet catch signals sent when the child dies.
 * in exit.c or in signal.c.
 */

/*
 * Called by kernel/ptrace.c when detaching..
 */
void ptrace_disable(struct task_struct *child)
{
	/*
	 * This would be better off in core code, but PTRACE_DETACH has
	 * grown its fair share of arch-specific worts and changing it
	 * is likely to cause regressions on obscure architectures.
	 */
	user_disable_single_step(child);
}


static int gpr_get(struct task_struct *target,
		   const struct user_regset *regset,
		   struct membuf to)
{
	struct user_pt_regs *uregs = &task_pt_regs(target)->user_regs;
	return membuf_write(&to, uregs, sizeof(*uregs));
}

static int gpr_set(struct task_struct *target, const struct user_regset *regset,
		   unsigned int pos, unsigned int count,
		   const void *kbuf, const void __user *ubuf)
{
	int ret;
	struct user_pt_regs newregs = task_pt_regs(target)->user_regs;

	ret = user_regset_copyin(&pos, &count, &kbuf, &ubuf, &newregs, 0, -1);
	if (ret)
		return ret;

	if (!valid_user_regs(&newregs, target))
		return -EINVAL;

	task_pt_regs(target)->user_regs = newregs;
	return 0;
}

static int fpr_active(struct task_struct *target, const struct user_regset *regset)
{
	if (!system_supports_fpsimd())
		return -ENODEV;
	return regset->n;
}

/*
 * TODO: update fp accessors for lazy context switching (sync/flush hwstate)
 */
static int __fpr_get(struct task_struct *target,
		     const struct user_regset *regset,
		     struct membuf to)
{
	struct user_fpsimd_state *uregs;

	fpsimd_sync_from_effective_state(target);

	uregs = &target->thread.uw.fpsimd_state;

	return membuf_write(&to, uregs, sizeof(*uregs));
}

static int fpr_get(struct task_struct *target, const struct user_regset *regset,
		   struct membuf to)
{
	if (!system_supports_fpsimd())
		return -EINVAL;

	if (target == current)
		fpsimd_preserve_current_state();

	return __fpr_get(target, regset, to);
}

static int __fpr_set(struct task_struct *target,
		     const struct user_regset *regset,
		     unsigned int pos, unsigned int count,
		     const void *kbuf, const void __user *ubuf,
		     unsigned int start_pos)
{
	int ret;
	struct user_fpsimd_state newstate;

	/*
	 * Ensure target->thread.uw.fpsimd_state is up to date, so that a
	 * short copyin can't resurrect stale data.
	 */
	fpsimd_sync_from_effective_state(target);

	newstate = target->thread.uw.fpsimd_state;

	ret = user_regset_copyin(&pos, &count, &kbuf, &ubuf, &newstate,
				 start_pos, start_pos + sizeof(newstate));
	if (ret)
		return ret;

	target->thread.uw.fpsimd_state = newstate;

	return ret;
}

static int fpr_set(struct task_struct *target, const struct user_regset *regset,
		   unsigned int pos, unsigned int count,
		   const void *kbuf, const void __user *ubuf)
{
	int ret;

	if (!system_supports_fpsimd())
		return -EINVAL;

	ret = __fpr_set(target, regset, pos, count, kbuf, ubuf, 0);
	if (ret)
		return ret;

	fpsimd_sync_to_effective_state_zeropad(target);
	fpsimd_flush_task_state(target);

	return ret;
}

static int tls_get(struct task_struct *target, const struct user_regset *regset,
		   struct membuf to)
{
	int ret;

	if (target == current)
		tls_preserve_current_state();

	ret = membuf_store(&to, target->thread.uw.tp_value);
	if (system_supports_tpidr2())
		ret = membuf_store(&to, target->thread.tpidr2_el0);
	else
		ret = membuf_zero(&to, sizeof(u64));

	return ret;
}

static int tls_set(struct task_struct *target, const struct user_regset *regset,
		   unsigned int pos, unsigned int count,
		   const void *kbuf, const void __user *ubuf)
{
	int ret;
	unsigned long tls[2];

	tls[0] = target->thread.uw.tp_value;
	if (system_supports_tpidr2())
		tls[1] = target->thread.tpidr2_el0;

	ret = user_regset_copyin(&pos, &count, &kbuf, &ubuf, tls, 0, count);
	if (ret)
		return ret;

	target->thread.uw.tp_value = tls[0];
	if (system_supports_tpidr2())
		target->thread.tpidr2_el0 = tls[1];

	return ret;
}

static int fpmr_get(struct task_struct *target, const struct user_regset *regset,
		   struct membuf to)
{
	if (!system_supports_fpmr())
		return -EINVAL;

	if (target == current)
		fpsimd_preserve_current_state();

	return membuf_store(&to, target->thread.uw.fpmr);
}

static int fpmr_set(struct task_struct *target, const struct user_regset *regset,
		   unsigned int pos, unsigned int count,
		   const void *kbuf, const void __user *ubuf)
{
	int ret;
	unsigned long fpmr;

	if (!system_supports_fpmr())
		return -EINVAL;

	fpmr = target->thread.uw.fpmr;

	ret = user_regset_copyin(&pos, &count, &kbuf, &ubuf, &fpmr, 0, count);
	if (ret)
		return ret;

	target->thread.uw.fpmr = fpmr;

	fpsimd_flush_task_state(target);

	return 0;
}

static int system_call_get(struct task_struct *target,
			   const struct user_regset *regset,
			   struct membuf to)
{
	return membuf_store(&to, task_pt_regs(target)->syscallno);
}

static int system_call_set(struct task_struct *target,
			   const struct user_regset *regset,
			   unsigned int pos, unsigned int count,
			   const void *kbuf, const void __user *ubuf)
{
	int syscallno = task_pt_regs(target)->syscallno;
	int ret;

	ret = user_regset_copyin(&pos, &count, &kbuf, &ubuf, &syscallno, 0, -1);
	if (ret)
		return ret;

	task_pt_regs(target)->syscallno = syscallno;
	return ret;
}

#ifdef CONFIG_ARM64_SVE

static void sve_init_header_from_task(struct user_sve_header *header,
				      struct task_struct *target,
				      enum vec_type type)
{
	unsigned int vq;
	bool active;
	enum vec_type task_type;

	memset(header, 0, sizeof(*header));

	/* Check if the requested registers are active for the task */
	if (thread_sm_enabled(&target->thread))
		task_type = ARM64_VEC_SME;
	else
		task_type = ARM64_VEC_SVE;
	active = (task_type == type);

	if (active && target->thread.fp_type == FP_STATE_SVE)
		header->flags = SVE_PT_REGS_SVE;
	else
		header->flags = SVE_PT_REGS_FPSIMD;

	switch (type) {
	case ARM64_VEC_SVE:
		if (test_tsk_thread_flag(target, TIF_SVE_VL_INHERIT))
			header->flags |= SVE_PT_VL_INHERIT;
		break;
	case ARM64_VEC_SME:
		if (test_tsk_thread_flag(target, TIF_SME_VL_INHERIT))
			header->flags |= SVE_PT_VL_INHERIT;
		break;
	default:
		WARN_ON_ONCE(1);
		return;
	}

	header->vl = task_get_vl(target, type);
	vq = sve_vq_from_vl(header->vl);

	header->max_vl = vec_max_vl(type);
	if (active)
		header->size = SVE_PT_SIZE(vq, header->flags);
	else
		header->size = sizeof(header);
	header->max_size = SVE_PT_SIZE(sve_vq_from_vl(header->max_vl),
				      SVE_PT_REGS_SVE);
}

static unsigned int sve_size_from_header(struct user_sve_header const *header)
{
	return ALIGN(header->size, SVE_VQ_BYTES);
}

static int sve_get_common(struct task_struct *target,
			  const struct user_regset *regset,
			  struct membuf to,
			  enum vec_type type)
{
	struct user_sve_header header;
	unsigned int vq;
	unsigned long start, end;

	if (target == current)
		fpsimd_preserve_current_state();

	/* Header */
	sve_init_header_from_task(&header, target, type);
	vq = sve_vq_from_vl(header.vl);

	membuf_write(&to, &header, sizeof(header));

	BUILD_BUG_ON(SVE_PT_FPSIMD_OFFSET != sizeof(header));
	BUILD_BUG_ON(SVE_PT_SVE_OFFSET != sizeof(header));

	/*
	 * When the requested vector type is not active, do not present data
	 * from the other mode to userspace.
	 */
	if (header.size == sizeof(header))
		return 0;

	switch ((header.flags & SVE_PT_REGS_MASK)) {
	case SVE_PT_REGS_FPSIMD:
		return __fpr_get(target, regset, to);

	case SVE_PT_REGS_SVE:
		start = SVE_PT_SVE_OFFSET;
		end = SVE_PT_SVE_FFR_OFFSET(vq) + SVE_PT_SVE_FFR_SIZE(vq);
		membuf_write(&to, target->thread.sve_state, end - start);

		start = end;
		end = SVE_PT_SVE_FPSR_OFFSET(vq);
		membuf_zero(&to, end - start);

		/*
		 * Copy fpsr, and fpcr which must follow contiguously in
		 * struct fpsimd_state:
		 */
		start = end;
		end = SVE_PT_SVE_FPCR_OFFSET(vq) + SVE_PT_SVE_FPCR_SIZE;
		membuf_write(&to, &target->thread.uw.fpsimd_state.fpsr,
			     end - start);

		start = end;
		end = sve_size_from_header(&header);
		return membuf_zero(&to, end - start);

	default:
		BUILD_BUG();
	}
}

static int sve_get(struct task_struct *target,
		   const struct user_regset *regset,
		   struct membuf to)
{
	if (!system_supports_sve())
		return -EINVAL;

	return sve_get_common(target, regset, to, ARM64_VEC_SVE);
}

static int sve_set_common(struct task_struct *target,
			  const struct user_regset *regset,
			  unsigned int pos, unsigned int count,
			  const void *kbuf, const void __user *ubuf,
			  enum vec_type type)
{
	int ret;
	struct user_sve_header header;
	unsigned int vq;
	unsigned long start, end;
	bool fpsimd;

	fpsimd_flush_task_state(target);

	/* Header */
	if (count < sizeof(header))
		return -EINVAL;
	ret = user_regset_copyin(&pos, &count, &kbuf, &ubuf, &header,
				 0, sizeof(header));
	if (ret)
		return ret;

	/*
	 * Streaming SVE data is always stored and presented in SVE format.
	 * Require the user to provide SVE formatted data for consistency, and
	 * to avoid the risk that we configure the task into an invalid state.
	 */
	fpsimd = (header.flags & SVE_PT_REGS_MASK) == SVE_PT_REGS_FPSIMD;
	if (fpsimd && type == ARM64_VEC_SME)
		return -EINVAL;

	/*
	 * On systems without SVE we accept FPSIMD format writes with
	 * a VL of 0 to allow exiting streaming mode, otherwise a VL
	 * is required.
	 */
	if (header.vl) {
		/*
		 * If the system does not support SVE we can't
		 * configure a SVE VL.
		 */
		if (!system_supports_sve() && type == ARM64_VEC_SVE)
			return -EINVAL;

		/*
		 * Apart from SVE_PT_REGS_MASK, all SVE_PT_* flags are
		 * consumed by vec_set_vector_length(), which will
		 * also validate them for us:
		 */
		ret = vec_set_vector_length(target, type, header.vl,
					    ((unsigned long)header.flags & ~SVE_PT_REGS_MASK) << 16);
		if (ret)
			return ret;
	} else {
		/* If the system supports SVE we require a VL. */
		if (system_supports_sve())
			return -EINVAL;

		/*
		 * Only FPSIMD formatted data with no flags set is
		 * supported.
		 */
		if (header.flags != SVE_PT_REGS_FPSIMD)
			return -EINVAL;
	}

	/* Allocate SME storage if necessary, preserving any existing ZA/ZT state */
	if (type == ARM64_VEC_SME) {
		sme_alloc(target, false);
		if (!target->thread.sme_state)
			return -ENOMEM;
	}

	/* Allocate SVE storage if necessary, zeroing any existing SVE state */
	if (!fpsimd) {
		sve_alloc(target, true);
		if (!target->thread.sve_state)
			return -ENOMEM;
	}

	/*
	 * Actual VL set may be different from what the user asked
	 * for, or we may have configured the _ONEXEC VL not the
	 * current VL:
	 */
	vq = sve_vq_from_vl(task_get_vl(target, type));

	/* Enter/exit streaming mode */
	switch (type) {
	case ARM64_VEC_SVE:
		target->thread.svcr &= ~SVCR_SM_MASK;
		set_tsk_thread_flag(target, TIF_SVE);
		break;
	case ARM64_VEC_SME:
		target->thread.svcr |= SVCR_SM_MASK;
		set_tsk_thread_flag(target, TIF_SME);
		break;
	default:
		WARN_ON_ONCE(1);
		return -EINVAL;
	}

	/* Always zero V regs, FPSR, and FPCR */
	memset(&current->thread.uw.fpsimd_state, 0,
	       sizeof(current->thread.uw.fpsimd_state));

	/* Registers: FPSIMD-only case */

	BUILD_BUG_ON(SVE_PT_FPSIMD_OFFSET != sizeof(header));
	if (fpsimd) {
		clear_tsk_thread_flag(target, TIF_SVE);
		target->thread.fp_type = FP_STATE_FPSIMD;
		ret = __fpr_set(target, regset, pos, count, kbuf, ubuf,
				SVE_PT_FPSIMD_OFFSET);
		return ret;
	}

	/* Otherwise: no registers or full SVE case. */

	target->thread.fp_type = FP_STATE_SVE;

	/*
	 * If setting a different VL from the requested VL and there is
	 * register data, the data layout will be wrong: don't even
	 * try to set the registers in this case.
	 */
	if (count && vq != sve_vq_from_vl(header.vl))
		return -EIO;

	BUILD_BUG_ON(SVE_PT_SVE_OFFSET != sizeof(header));
	start = SVE_PT_SVE_OFFSET;
	end = SVE_PT_SVE_FFR_OFFSET(vq) + SVE_PT_SVE_FFR_SIZE(vq);
	ret = user_regset_copyin(&pos, &count, &kbuf, &ubuf,
				 target->thread.sve_state,
				 start, end);
	if (ret)
		return ret;

	start = end;
	end = SVE_PT_SVE_FPSR_OFFSET(vq);
	user_regset_copyin_ignore(&pos, &count, &kbuf, &ubuf, start, end);

	/*
	 * Copy fpsr, and fpcr which must follow contiguously in
	 * struct fpsimd_state:
	 */
	start = end;
	end = SVE_PT_SVE_FPCR_OFFSET(vq) + SVE_PT_SVE_FPCR_SIZE;
	ret = user_regset_copyin(&pos, &count, &kbuf, &ubuf,
				 &target->thread.uw.fpsimd_state.fpsr,
				 start, end);

	return ret;
}

static int sve_set(struct task_struct *target,
		   const struct user_regset *regset,
		   unsigned int pos, unsigned int count,
		   const void *kbuf, const void __user *ubuf)
{
	if (!system_supports_sve() && !system_supports_sme())
		return -EINVAL;

	return sve_set_common(target, regset, pos, count, kbuf, ubuf,
			      ARM64_VEC_SVE);
}

#endif /* CONFIG_ARM64_SVE */

#ifdef CONFIG_ARM64_SME

static int ssve_get(struct task_struct *target,
		   const struct user_regset *regset,
		   struct membuf to)
{
	if (!system_supports_sme())
		return -EINVAL;

	return sve_get_common(target, regset, to, ARM64_VEC_SME);
}

static int ssve_set(struct task_struct *target,
		    const struct user_regset *regset,
		    unsigned int pos, unsigned int count,
		    const void *kbuf, const void __user *ubuf)
{
	if (!system_supports_sme())
		return -EINVAL;

	return sve_set_common(target, regset, pos, count, kbuf, ubuf,
			      ARM64_VEC_SME);
}

static int za_get(struct task_struct *target,
		  const struct user_regset *regset,
		  struct membuf to)
{
	struct user_za_header header;
	unsigned int vq;
	unsigned long start, end;

	if (!system_supports_sme())
		return -EINVAL;

	/* Header */
	memset(&header, 0, sizeof(header));

	if (test_tsk_thread_flag(target, TIF_SME_VL_INHERIT))
		header.flags |= ZA_PT_VL_INHERIT;

	header.vl = task_get_sme_vl(target);
	vq = sve_vq_from_vl(header.vl);
	header.max_vl = sme_max_vl();
	header.max_size = ZA_PT_SIZE(vq);

	/* If ZA is not active there is only the header */
	if (thread_za_enabled(&target->thread))
		header.size = ZA_PT_SIZE(vq);
	else
		header.size = ZA_PT_ZA_OFFSET;

	membuf_write(&to, &header, sizeof(header));

	BUILD_BUG_ON(ZA_PT_ZA_OFFSET != sizeof(header));
	end = ZA_PT_ZA_OFFSET;

	if (target == current)
		fpsimd_preserve_current_state();

	/* Any register data to include? */
	if (thread_za_enabled(&target->thread)) {
		start = end;
		end = ZA_PT_SIZE(vq);
		membuf_write(&to, target->thread.sme_state, end - start);
	}

	/* Zero any trailing padding */
	start = end;
	end = ALIGN(header.size, SVE_VQ_BYTES);
	return membuf_zero(&to, end - start);
}

static int za_set(struct task_struct *target,
		  const struct user_regset *regset,
		  unsigned int pos, unsigned int count,
		  const void *kbuf, const void __user *ubuf)
{
	int ret;
	struct user_za_header header;
	unsigned int vq;
	unsigned long start, end;

	if (!system_supports_sme())
		return -EINVAL;

	/* Header */
	if (count < sizeof(header))
		return -EINVAL;
	ret = user_regset_copyin(&pos, &count, &kbuf, &ubuf, &header,
				 0, sizeof(header));
	if (ret)
		goto out;

	/*
	 * All current ZA_PT_* flags are consumed by
	 * vec_set_vector_length(), which will also validate them for
	 * us:
	 */
	ret = vec_set_vector_length(target, ARM64_VEC_SME, header.vl,
		((unsigned long)header.flags) << 16);
	if (ret)
		goto out;

	/*
	 * Actual VL set may be different from what the user asked
	 * for, or we may have configured the _ONEXEC rather than
	 * current VL:
	 */
	vq = sve_vq_from_vl(task_get_sme_vl(target));

	/* Ensure there is some SVE storage for streaming mode */
	if (!target->thread.sve_state) {
		sve_alloc(target, false);
		if (!target->thread.sve_state) {
			ret = -ENOMEM;
			goto out;
		}
	}

	/*
	 * Only flush the storage if PSTATE.ZA was not already set,
	 * otherwise preserve any existing data.
	 */
	sme_alloc(target, !thread_za_enabled(&target->thread));
	if (!target->thread.sme_state)
		return -ENOMEM;

	/* If there is no data then disable ZA */
	if (!count) {
		target->thread.svcr &= ~SVCR_ZA_MASK;
		goto out;
	}

	/*
	 * If setting a different VL from the requested VL and there is
	 * register data, the data layout will be wrong: don't even
	 * try to set the registers in this case.
	 */
	if (vq != sve_vq_from_vl(header.vl)) {
		ret = -EIO;
		goto out;
	}

	BUILD_BUG_ON(ZA_PT_ZA_OFFSET != sizeof(header));
	start = ZA_PT_ZA_OFFSET;
	end = ZA_PT_SIZE(vq);
	ret = user_regset_copyin(&pos, &count, &kbuf, &ubuf,
				 target->thread.sme_state,
				 start, end);
	if (ret)
		goto out;

	/* Mark ZA as active and let userspace use it */
	set_tsk_thread_flag(target, TIF_SME);
	target->thread.svcr |= SVCR_ZA_MASK;

out:
	fpsimd_flush_task_state(target);
	return ret;
}

static int zt_get(struct task_struct *target,
		  const struct user_regset *regset,
		  struct membuf to)
{
	if (!system_supports_sme2())
		return -EINVAL;

	/*
	 * If PSTATE.ZA is not set then ZT will be zeroed when it is
	 * enabled so report the current register value as zero.
	 */
	if (thread_za_enabled(&target->thread))
		membuf_write(&to, thread_zt_state(&target->thread),
			     ZT_SIG_REG_BYTES);
	else
		membuf_zero(&to, ZT_SIG_REG_BYTES);

	return 0;
}

static int zt_set(struct task_struct *target,
		  const struct user_regset *regset,
		  unsigned int pos, unsigned int count,
		  const void *kbuf, const void __user *ubuf)
{
	int ret;

	if (!system_supports_sme2())
		return -EINVAL;

	/* Ensure SVE storage in case this is first use of SME */
	sve_alloc(target, false);
	if (!target->thread.sve_state)
		return -ENOMEM;

	if (!thread_za_enabled(&target->thread)) {
		sme_alloc(target, true);
		if (!target->thread.sme_state)
			return -ENOMEM;
	}

	ret = user_regset_copyin(&pos, &count, &kbuf, &ubuf,
				 thread_zt_state(&target->thread),
				 0, ZT_SIG_REG_BYTES);
	if (ret == 0) {
		target->thread.svcr |= SVCR_ZA_MASK;
		set_tsk_thread_flag(target, TIF_SME);
	}

	fpsimd_flush_task_state(target);

	return ret;
}

#endif /* CONFIG_ARM64_SME */


#ifdef CONFIG_ARM64_TAGGED_ADDR_ABI
static int tagged_addr_ctrl_get(struct task_struct *target,
				const struct user_regset *regset,
				struct membuf to)
{
	long ctrl = get_tagged_addr_ctrl(target);

	if (WARN_ON_ONCE(IS_ERR_VALUE(ctrl)))
		return ctrl;

	return membuf_write(&to, &ctrl, sizeof(ctrl));
}

static int tagged_addr_ctrl_set(struct task_struct *target, const struct
				user_regset *regset, unsigned int pos,
				unsigned int count, const void *kbuf, const
				void __user *ubuf)
{
	int ret;
	long ctrl;

	ctrl = get_tagged_addr_ctrl(target);
	if (WARN_ON_ONCE(IS_ERR_VALUE(ctrl)))
		return ctrl;

	ret = user_regset_copyin(&pos, &count, &kbuf, &ubuf, &ctrl, 0, -1);
	if (ret)
		return ret;

	return set_tagged_addr_ctrl(target, ctrl);
}
#endif

#ifdef CONFIG_ARM64_POE
static int poe_get(struct task_struct *target,
		   const struct user_regset *regset,
		   struct membuf to)
{
	if (!system_supports_poe())
		return -EINVAL;

	if (target == current)
		current->thread.por_el0 = read_sysreg_s(SYS_POR_EL0);

	return membuf_write(&to, &target->thread.por_el0,
			    sizeof(target->thread.por_el0));
}

static int poe_set(struct task_struct *target, const struct
		   user_regset *regset, unsigned int pos,
		   unsigned int count, const void *kbuf, const
		   void __user *ubuf)
{
	int ret;
	long ctrl;

	if (!system_supports_poe())
		return -EINVAL;

	ctrl = target->thread.por_el0;

	ret = user_regset_copyin(&pos, &count, &kbuf, &ubuf, &ctrl, 0, -1);
	if (ret)
		return ret;

	target->thread.por_el0 = ctrl;

	return 0;
}
#endif

#ifdef CONFIG_ARM64_GCS
static void task_gcs_to_user(struct user_gcs *user_gcs,
			     const struct task_struct *target)
{
	user_gcs->features_enabled = target->thread.gcs_el0_mode;
	user_gcs->features_locked = target->thread.gcs_el0_locked;
	user_gcs->gcspr_el0 = target->thread.gcspr_el0;
}

static void task_gcs_from_user(struct task_struct *target,
			       const struct user_gcs *user_gcs)
{
	target->thread.gcs_el0_mode = user_gcs->features_enabled;
	target->thread.gcs_el0_locked = user_gcs->features_locked;
	target->thread.gcspr_el0 = user_gcs->gcspr_el0;
}

static int gcs_get(struct task_struct *target,
		   const struct user_regset *regset,
		   struct membuf to)
{
	struct user_gcs user_gcs;

	if (!system_supports_gcs())
		return -EINVAL;

	if (target == current)
		gcs_preserve_current_state();

	task_gcs_to_user(&user_gcs, target);

	return membuf_write(&to, &user_gcs, sizeof(user_gcs));
}

static int gcs_set(struct task_struct *target, const struct
		   user_regset *regset, unsigned int pos,
		   unsigned int count, const void *kbuf, const
		   void __user *ubuf)
{
	int ret;
	struct user_gcs user_gcs;

	if (!system_supports_gcs())
		return -EINVAL;

	task_gcs_to_user(&user_gcs, target);

	ret = user_regset_copyin(&pos, &count, &kbuf, &ubuf, &user_gcs, 0, -1);
	if (ret)
		return ret;

	if (user_gcs.features_enabled & ~PR_SHADOW_STACK_SUPPORTED_STATUS_MASK)
		return -EINVAL;

	task_gcs_from_user(target, &user_gcs);

	return 0;
}
#endif

enum aarch64_regset {
	REGSET_GPR,
	REGSET_FPR,
	REGSET_TLS,
	REGSET_FPMR,
	REGSET_SYSTEM_CALL,
#ifdef CONFIG_ARM64_SVE
	REGSET_SVE,
#endif
#ifdef CONFIG_ARM64_SME
	REGSET_SSVE,
	REGSET_ZA,
	REGSET_ZT,
#endif
#ifdef CONFIG_ARM64_TAGGED_ADDR_ABI
	REGSET_TAGGED_ADDR_CTRL,
#endif
#ifdef CONFIG_ARM64_POE
	REGSET_POE,
#endif
#ifdef CONFIG_ARM64_GCS
	REGSET_GCS,
#endif
};

static const struct user_regset aarch64_regsets[] = {
	[REGSET_GPR] = {
		USER_REGSET_NOTE_TYPE(PRSTATUS),
		.n = sizeof(struct user_pt_regs) / sizeof(u64),
		.size = sizeof(u64),
		.align = sizeof(u64),
		.regset_get = gpr_get,
		.set = gpr_set
	},
	[REGSET_FPR] = {
		USER_REGSET_NOTE_TYPE(PRFPREG),
		.n = sizeof(struct user_fpsimd_state) / sizeof(u32),
		/*
		 * We pretend we have 32-bit registers because the fpsr and
		 * fpcr are 32-bits wide.
		 */
		.size = sizeof(u32),
		.align = sizeof(u32),
		.active = fpr_active,
		.regset_get = fpr_get,
		.set = fpr_set
	},
	[REGSET_TLS] = {
		USER_REGSET_NOTE_TYPE(ARM_TLS),
		.n = 2,
		.size = sizeof(void *),
		.align = sizeof(void *),
		.regset_get = tls_get,
		.set = tls_set,
	},
	[REGSET_SYSTEM_CALL] = {
		USER_REGSET_NOTE_TYPE(ARM_SYSTEM_CALL),
		.n = 1,
		.size = sizeof(int),
		.align = sizeof(int),
		.regset_get = system_call_get,
		.set = system_call_set,
	},
	[REGSET_FPMR] = {
		USER_REGSET_NOTE_TYPE(ARM_FPMR),
		.n = 1,
		.size = sizeof(u64),
		.align = sizeof(u64),
		.regset_get = fpmr_get,
		.set = fpmr_set,
	},
#ifdef CONFIG_ARM64_SVE
	[REGSET_SVE] = { /* Scalable Vector Extension */
		USER_REGSET_NOTE_TYPE(ARM_SVE),
		.n = DIV_ROUND_UP(SVE_PT_SIZE(ARCH_SVE_VQ_MAX,
					      SVE_PT_REGS_SVE),
				  SVE_VQ_BYTES),
		.size = SVE_VQ_BYTES,
		.align = SVE_VQ_BYTES,
		.regset_get = sve_get,
		.set = sve_set,
	},
#endif
#ifdef CONFIG_ARM64_SME
	[REGSET_SSVE] = { /* Streaming mode SVE */
		USER_REGSET_NOTE_TYPE(ARM_SSVE),
		.n = DIV_ROUND_UP(SVE_PT_SIZE(SME_VQ_MAX, SVE_PT_REGS_SVE),
				  SVE_VQ_BYTES),
		.size = SVE_VQ_BYTES,
		.align = SVE_VQ_BYTES,
		.regset_get = ssve_get,
		.set = ssve_set,
	},
	[REGSET_ZA] = { /* SME ZA */
		USER_REGSET_NOTE_TYPE(ARM_ZA),
		/*
		 * ZA is a single register but it's variably sized and
		 * the ptrace core requires that the size of any data
		 * be an exact multiple of the configured register
		 * size so report as though we had SVE_VQ_BYTES
		 * registers. These values aren't exposed to
		 * userspace.
		 */
		.n = DIV_ROUND_UP(ZA_PT_SIZE(SME_VQ_MAX), SVE_VQ_BYTES),
		.size = SVE_VQ_BYTES,
		.align = SVE_VQ_BYTES,
		.regset_get = za_get,
		.set = za_set,
	},
	[REGSET_ZT] = { /* SME ZT */
		USER_REGSET_NOTE_TYPE(ARM_ZT),
		.n = 1,
		.size = ZT_SIG_REG_BYTES,
		.align = sizeof(u64),
		.regset_get = zt_get,
		.set = zt_set,
	},
#endif
#ifdef CONFIG_ARM64_TAGGED_ADDR_ABI
	[REGSET_TAGGED_ADDR_CTRL] = {
		USER_REGSET_NOTE_TYPE(ARM_TAGGED_ADDR_CTRL),
		.n = 1,
		.size = sizeof(long),
		.align = sizeof(long),
		.regset_get = tagged_addr_ctrl_get,
		.set = tagged_addr_ctrl_set,
	},
#endif
#ifdef CONFIG_ARM64_POE
	[REGSET_POE] = {
		USER_REGSET_NOTE_TYPE(ARM_POE),
		.n = 1,
		.size = sizeof(long),
		.align = sizeof(long),
		.regset_get = poe_get,
		.set = poe_set,
	},
#endif
#ifdef CONFIG_ARM64_GCS
	[REGSET_GCS] = {
		USER_REGSET_NOTE_TYPE(ARM_GCS),
		.n = sizeof(struct user_gcs) / sizeof(u64),
		.size = sizeof(u64),
		.align = sizeof(u64),
		.regset_get = gcs_get,
		.set = gcs_set,
	},
#endif
};

static const struct user_regset_view user_aarch64_view = {
	.name = "aarch64", .e_machine = EM_AARCH64,
	.regsets = aarch64_regsets, .n = ARRAY_SIZE(aarch64_regsets)
};

enum compat_regset {
	REGSET_COMPAT_GPR,
	REGSET_COMPAT_VFP,
};

static inline compat_ulong_t compat_get_user_reg(struct task_struct *task, int idx)
{
	struct pt_regs *regs = task_pt_regs(task);

	switch (idx) {
	case 15:
		return regs->pc;
	case 16:
		return pstate_to_compat_psr(regs->pstate);
	case 17:
		return regs->orig_x0;
	default:
		return regs->regs[idx];
	}
}

static int compat_gpr_get(struct task_struct *target,
			  const struct user_regset *regset,
			  struct membuf to)
{
	int i = 0;

	while (to.left)
		membuf_store(&to, compat_get_user_reg(target, i++));
	return 0;
}

static int compat_gpr_set(struct task_struct *target,
			  const struct user_regset *regset,
			  unsigned int pos, unsigned int count,
			  const void *kbuf, const void __user *ubuf)
{
	struct pt_regs newregs;
	int ret = 0;
	unsigned int i, start, num_regs;

	/* Calculate the number of AArch32 registers contained in count */
	num_regs = count / regset->size;

	/* Convert pos into an register number */
	start = pos / regset->size;

	if (start + num_regs > regset->n)
		return -EIO;

	newregs = *task_pt_regs(target);

	for (i = 0; i < num_regs; ++i) {
		unsigned int idx = start + i;
		compat_ulong_t reg;

		if (kbuf) {
			memcpy(&reg, kbuf, sizeof(reg));
			kbuf += sizeof(reg);
		} else {
			ret = copy_from_user(&reg, ubuf, sizeof(reg));
			if (ret) {
				ret = -EFAULT;
				break;
			}

			ubuf += sizeof(reg);
		}

		switch (idx) {
		case 15:
			newregs.pc = reg;
			break;
		case 16:
			reg = compat_psr_to_pstate(reg);
			newregs.pstate = reg;
			break;
		case 17:
			newregs.orig_x0 = reg;
			break;
		default:
			newregs.regs[idx] = reg;
		}

	}

	if (valid_user_regs(&newregs.user_regs, target))
		*task_pt_regs(target) = newregs;
	else
		ret = -EINVAL;

	return ret;
}

static int compat_vfp_get(struct task_struct *target,
			  const struct user_regset *regset,
			  struct membuf to)
{
	struct user_fpsimd_state *uregs;
	compat_ulong_t fpscr;

	if (!system_supports_fpsimd())
		return -EINVAL;

	uregs = &target->thread.uw.fpsimd_state;

	if (target == current)
		fpsimd_preserve_current_state();

	/*
	 * The VFP registers are packed into the fpsimd_state, so they all sit
	 * nicely together for us. We just need to create the fpscr separately.
	 */
	membuf_write(&to, uregs, VFP_STATE_SIZE - sizeof(compat_ulong_t));
	fpscr = (uregs->fpsr & VFP_FPSCR_STAT_MASK) |
		(uregs->fpcr & VFP_FPSCR_CTRL_MASK);
	return membuf_store(&to, fpscr);
}

static int compat_vfp_set(struct task_struct *target,
			  const struct user_regset *regset,
			  unsigned int pos, unsigned int count,
			  const void *kbuf, const void __user *ubuf)
{
	struct user_fpsimd_state *uregs;
	compat_ulong_t fpscr;
	int ret, vregs_end_pos;

	if (!system_supports_fpsimd())
		return -EINVAL;

	uregs = &target->thread.uw.fpsimd_state;

	vregs_end_pos = VFP_STATE_SIZE - sizeof(compat_ulong_t);
	ret = user_regset_copyin(&pos, &count, &kbuf, &ubuf, uregs, 0,
				 vregs_end_pos);

	if (count && !ret) {
		ret = user_regset_copyin(&pos, &count, &kbuf, &ubuf, &fpscr,
					 vregs_end_pos, VFP_STATE_SIZE);
		if (!ret) {
			uregs->fpsr = fpscr & VFP_FPSCR_STAT_MASK;
			uregs->fpcr = fpscr & VFP_FPSCR_CTRL_MASK;
		}
	}

	fpsimd_flush_task_state(target);
	return ret;
}

static int compat_tls_get(struct task_struct *target,
			  const struct user_regset *regset,
			  struct membuf to)
{
	return membuf_store(&to, (compat_ulong_t)target->thread.uw.tp_value);
}

static int compat_tls_set(struct task_struct *target,
			  const struct user_regset *regset, unsigned int pos,
			  unsigned int count, const void *kbuf,
			  const void __user *ubuf)
{
	int ret;
	compat_ulong_t tls = target->thread.uw.tp_value;

	ret = user_regset_copyin(&pos, &count, &kbuf, &ubuf, &tls, 0, -1);
	if (ret)
		return ret;

	target->thread.uw.tp_value = tls;
	return ret;
}

static const struct user_regset aarch32_regsets[] = {
	[REGSET_COMPAT_GPR] = {
		USER_REGSET_NOTE_TYPE(PRSTATUS),
		.n = COMPAT_ELF_NGREG,
		.size = sizeof(compat_elf_greg_t),
		.align = sizeof(compat_elf_greg_t),
		.regset_get = compat_gpr_get,
		.set = compat_gpr_set
	},
	[REGSET_COMPAT_VFP] = {
		USER_REGSET_NOTE_TYPE(ARM_VFP),
		.n = VFP_STATE_SIZE / sizeof(compat_ulong_t),
		.size = sizeof(compat_ulong_t),
		.align = sizeof(compat_ulong_t),
		.active = fpr_active,
		.regset_get = compat_vfp_get,
		.set = compat_vfp_set
	},
};

static const struct user_regset_view user_aarch32_view = {
	.name = "aarch32", .e_machine = EM_ARM,
	.regsets = aarch32_regsets, .n = ARRAY_SIZE(aarch32_regsets)
};

static const struct user_regset aarch32_ptrace_regsets[] = {
	[REGSET_GPR] = {
		USER_REGSET_NOTE_TYPE(PRSTATUS),
		.n = COMPAT_ELF_NGREG,
		.size = sizeof(compat_elf_greg_t),
		.align = sizeof(compat_elf_greg_t),
		.regset_get = compat_gpr_get,
		.set = compat_gpr_set
	},
	[REGSET_FPR] = {
		USER_REGSET_NOTE_TYPE(ARM_VFP),
		.n = VFP_STATE_SIZE / sizeof(compat_ulong_t),
		.size = sizeof(compat_ulong_t),
		.align = sizeof(compat_ulong_t),
		.regset_get = compat_vfp_get,
		.set = compat_vfp_set
	},
	[REGSET_TLS] = {
		USER_REGSET_NOTE_TYPE(ARM_TLS),
		.n = 1,
		.size = sizeof(compat_ulong_t),
		.align = sizeof(compat_ulong_t),
		.regset_get = compat_tls_get,
		.set = compat_tls_set,
	},
	[REGSET_SYSTEM_CALL] = {
		USER_REGSET_NOTE_TYPE(ARM_SYSTEM_CALL),
		.n = 1,
		.size = sizeof(int),
		.align = sizeof(int),
		.regset_get = system_call_get,
		.set = system_call_set,
	},
};

static const struct user_regset_view user_aarch32_ptrace_view = {
	.name = "aarch32", .e_machine = EM_ARM,
	.regsets = aarch32_ptrace_regsets, .n = ARRAY_SIZE(aarch32_ptrace_regsets)
};


const struct user_regset_view *task_user_regset_view(struct task_struct *task)
{
	/*
	 * Core dumping of 32-bit tasks or compat ptrace requests must use the
	 * user_aarch32_view compatible with arm32. Native ptrace requests on
	 * 32-bit children use an extended user_aarch32_ptrace_view to allow
	 * access to the TLS register.
	 */
	if (is_compat_task())
		return &user_aarch32_view;
	else if (is_compat_thread(task_thread_info(task)))
		return &user_aarch32_ptrace_view;

	return &user_aarch64_view;
}

long arch_ptrace(struct task_struct *child, long request,
		 unsigned long addr, unsigned long data)
{
	switch (request) {
	case PTRACE_PEEKMTETAGS:
	case PTRACE_POKEMTETAGS:
		return mte_ptrace_copy_tags(child, request, addr, data);
	}

	return ptrace_request(child, request, addr, data);
}

enum ptrace_syscall_dir {
	PTRACE_SYSCALL_ENTER = 0,
	PTRACE_SYSCALL_EXIT,
};

static __always_inline unsigned long ptrace_save_reg(struct pt_regs *regs,
						     enum ptrace_syscall_dir dir,
						     int *regno)
{
	unsigned long saved_reg;

	/*
	 * We have some ABI weirdness here in the way that we handle syscall
	 * exit stops because we indicate whether or not the stop has been
	 * signalled from syscall entry or syscall exit by clobbering a general
	 * purpose register (ip/r12 for AArch32, x7 for AArch64) in the tracee
	 * and restoring its old value after the stop. This means that:
	 *
	 * - Any writes by the tracer to this register during the stop are
	 *   ignored/discarded.
	 *
	 * - The actual value of the register is not available during the stop,
	 *   so the tracer cannot save it and restore it later.
	 *
	 * - Syscall stops behave differently to seccomp and pseudo-step traps
	 *   (the latter do not nobble any registers).
	 */
	*regno = (is_compat_task() ? 12 : 7);
	saved_reg = regs->regs[*regno];
	regs->regs[*regno] = dir;

	return saved_reg;
}

static int report_syscall_entry(struct pt_regs *regs)
{
	unsigned long saved_reg;
	int regno, ret;

	saved_reg = ptrace_save_reg(regs, PTRACE_SYSCALL_ENTER, &regno);
	ret = ptrace_report_syscall_entry(regs);
	if (ret)
		forget_syscall(regs);
	regs->regs[regno] = saved_reg;

	return ret;
}

static void report_syscall_exit(struct pt_regs *regs)
{
	unsigned long saved_reg;
	int regno;

	saved_reg = ptrace_save_reg(regs, PTRACE_SYSCALL_EXIT, &regno);
	if (!test_thread_flag(TIF_SINGLESTEP)) {
		ptrace_report_syscall_exit(regs, 0);
		regs->regs[regno] = saved_reg;
	} else {
		regs->regs[regno] = saved_reg;

		/*
		 * Signal a pseudo-step exception since we are stepping but
		 * tracer modifications to the registers may have rewound the
		 * state machine.
		 */
		ptrace_report_syscall_exit(regs, 1);
	}
}

int syscall_trace_enter(struct pt_regs *regs)
{
	unsigned long flags = read_thread_flags();
	int ret;

	if (flags & (_TIF_SYSCALL_EMU | _TIF_SYSCALL_TRACE)) {
		ret = report_syscall_entry(regs);
		if (ret || (flags & _TIF_SYSCALL_EMU))
			return NO_SYSCALL;
	}

	/* Do the secure computing after ptrace; failures should be fast. */
	if (secure_computing() == -1)
		return NO_SYSCALL;

	return regs->syscallno;
}

void syscall_trace_exit(struct pt_regs *regs)
{
	unsigned long flags = read_thread_flags();

	if (flags & (_TIF_SYSCALL_TRACE | _TIF_SINGLESTEP))
		report_syscall_exit(regs);

	rseq_syscall(regs);
}

/*
 * SPSR_ELx bits which are always architecturally RES0 per ARM DDI 0487D.a.
 * We permit userspace to set SSBS (AArch64 bit 12, AArch32 bit 23) which is
 * not described in ARM DDI 0487D.a.
 * We treat PAN and UAO as RES0 bits, as they are meaningless at EL0, and may
 * be allocated an EL0 meaning in future.
 * Userspace cannot use these until they have an architectural meaning.
 * Note that this follows the SPSR_ELx format, not the AArch32 PSR format.
 * We also reserve IL for the kernel; SS is handled dynamically.
 */
#define SPSR_EL1_AARCH64_RES0_BITS \
	(GENMASK_ULL(63, 32) | GENMASK_ULL(27, 26) | GENMASK_ULL(23, 22) | \
	 GENMASK_ULL(20, 13) | GENMASK_ULL(5, 5))
#define SPSR_EL1_AARCH32_RES0_BITS \
	(GENMASK_ULL(63, 32) | GENMASK_ULL(22, 22) | GENMASK_ULL(20, 20))

static int valid_compat_regs(struct user_pt_regs *regs)
{
	regs->pstate &= ~SPSR_EL1_AARCH32_RES0_BITS;

	if (!system_supports_mixed_endian_el0())
		regs->pstate &= ~PSR_AA32_E_BIT;

	if (user_mode(regs) && (regs->pstate & PSR_MODE32_BIT) &&
	    (regs->pstate & PSR_AA32_A_BIT) == 0 &&
	    (regs->pstate & PSR_AA32_I_BIT) == 0 &&
	    (regs->pstate & PSR_AA32_F_BIT) == 0) {
		return 1;
	}

	/*
	 * Force PSR to a valid 32-bit EL0t, preserving the same bits as
	 * arch/arm.
	 */
	regs->pstate &= PSR_AA32_N_BIT | PSR_AA32_Z_BIT |
			PSR_AA32_C_BIT | PSR_AA32_V_BIT |
			PSR_AA32_Q_BIT | PSR_AA32_IT_MASK |
			PSR_AA32_GE_MASK | PSR_AA32_E_BIT |
			PSR_AA32_T_BIT;
	regs->pstate |= PSR_MODE32_BIT;

	return 0;
}

static int valid_native_regs(struct user_pt_regs *regs)
{
	regs->pstate &= ~SPSR_EL1_AARCH64_RES0_BITS;

	if (user_mode(regs) && !(regs->pstate & PSR_MODE32_BIT) &&
	    (regs->pstate & PSR_D_BIT) == 0 &&
	    (regs->pstate & PSR_A_BIT) == 0 &&
	    (regs->pstate & PSR_I_BIT) == 0 &&
	    (regs->pstate & PSR_F_BIT) == 0) {
		return 1;
	}

	/* Force PSR to a valid 64-bit EL0t */
	regs->pstate &= PSR_N_BIT | PSR_Z_BIT | PSR_C_BIT | PSR_V_BIT;

	return 0;
}

/*
 * Are the current registers suitable for user mode? (used to maintain
 * security in signal handlers)
 */
int valid_user_regs(struct user_pt_regs *regs, struct task_struct *task)
{
	/* https://lore.kernel.org/lkml/20191118131525.GA4180@willie-the-truck */
	user_regs_reset_single_step(regs, task);

	if (is_compat_thread(task_thread_info(task)))
		return valid_compat_regs(regs);
	else
		return valid_native_regs(regs);
}
