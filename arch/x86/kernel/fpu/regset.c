// SPDX-License-Identifier: GPL-2.0
/*
 * FPU register's regset abstraction, for ptrace, core dumps, etc.
 */
#include <linux/sched/task_stack.h>
#include <linux/vmalloc.h>

#include <asm/fpu/api.h>
#include <asm/fpu/signal.h>
#include <asm/fpu/regset.h>
#include <asm/prctl.h>

#include "context.h"
#include "internal.h"
#include "legacy.h"
#include "xstate.h"

/*
 * The xstateregs_active() routine is the same as the regset_fpregs_active() routine,
 * as the "regset->n" for the xstate regset will be updated based on the feature
 * capabilities supported by the xsave.
 */
int regset_fpregs_active(struct task_struct *target, const struct user_regset *regset)
{
	return regset->n;
}

int regset_xregset_fpregs_active(struct task_struct *target, const struct user_regset *regset)
{
	if (boot_cpu_has(X86_FEATURE_FXSR))
		return regset->n;
	else
		return 0;
}

/*
 * The regset get() functions are invoked from:
 *
 *   - coredump to dump the current task's fpstate. If the current task
 *     owns the FPU then the memory state has to be synchronized and the
 *     FPU register state preserved. Otherwise fpstate is already in sync.
 *
 *   - ptrace to dump fpstate of a stopped task, in which case the registers
 *     have already been saved to fpstate on context switch.
 */
static void sync_fpstate(struct fpu *fpu)
{
	if (fpu == x86_task_fpu(current))
		fpu_sync_fpstate(fpu);
}

/*
 * Invalidate cached FPU registers before modifying the stopped target
 * task's fpstate.
 *
 * This forces the target task on resume to restore the FPU registers from
 * modified fpstate. Otherwise the task might skip the restore and operate
 * with the cached FPU registers which discards the modifications.
 */
static void fpu_force_restore(struct fpu *fpu)
{
	/*
	 * Only stopped child tasks can be used to modify the FPU
	 * state in the fpstate buffer:
	 */
	WARN_ON_FPU(fpu == x86_task_fpu(current));

	__fpu_invalidate_fpregs_state(fpu);
}

int xfpregs_get(struct task_struct *target, const struct user_regset *regset,
		struct membuf to)
{
	struct fpu *fpu = x86_task_fpu(target);

	if (!cpu_feature_enabled(X86_FEATURE_FXSR))
		return -ENODEV;

	sync_fpstate(fpu);

	if (!use_xsave()) {
		return membuf_write(&to, &fpu->fpstate->regs.fxsave,
				    sizeof(fpu->fpstate->regs.fxsave));
	}

	copy_xstate_to_uabi_buf(to, target, XSTATE_COPY_FX);
	return 0;
}

int xfpregs_set(struct task_struct *target, const struct user_regset *regset,
		unsigned int pos, unsigned int count,
		const void *kbuf, const void __user *ubuf)
{
	struct fpu *fpu = x86_task_fpu(target);
	struct fxregs_state newstate;
	int ret;

	if (!cpu_feature_enabled(X86_FEATURE_FXSR))
		return -ENODEV;

	/* No funny business with partial or oversized writes is permitted. */
	if (pos != 0 || count != sizeof(newstate))
		return -EINVAL;

	ret = user_regset_copyin(&pos, &count, &kbuf, &ubuf, &newstate, 0, -1);
	if (ret)
		return ret;

	/* Do not allow an invalid MXCSR value. */
	if (newstate.mxcsr & ~mxcsr_feature_mask)
		return -EINVAL;

	fpu_force_restore(fpu);

	/* Copy the state  */
	memcpy(&fpu->fpstate->regs.fxsave, &newstate, sizeof(newstate));

	/* Clear xmm8..15 for 32-bit callers */
	BUILD_BUG_ON(sizeof(fpu->__fpstate.regs.fxsave.xmm_space) != 16 * 16);
	if (in_ia32_syscall())
		memset(&fpu->fpstate->regs.fxsave.xmm_space[8*4], 0, 8 * 16);

	/* Mark FP and SSE as in use when XSAVE is enabled */
	if (use_xsave())
		fpu->fpstate->regs.xsave.header.xfeatures |= XFEATURE_MASK_FPSSE;

	return 0;
}

int xstateregs_get(struct task_struct *target, const struct user_regset *regset,
		struct membuf to)
{
	if (!cpu_feature_enabled(X86_FEATURE_XSAVE))
		return -ENODEV;

	sync_fpstate(x86_task_fpu(target));

	copy_xstate_to_uabi_buf(to, target, XSTATE_COPY_XSAVE);
	return 0;
}

int xstateregs_set(struct task_struct *target, const struct user_regset *regset,
		  unsigned int pos, unsigned int count,
		  const void *kbuf, const void __user *ubuf)
{
	struct fpu *fpu = x86_task_fpu(target);
	struct xregs_state *tmpbuf = NULL;
	int ret;

	if (!cpu_feature_enabled(X86_FEATURE_XSAVE))
		return -ENODEV;

	/*
	 * A whole standard-format XSAVE buffer is needed:
	 */
	if (pos != 0 || count != fpu_user_cfg.max_size)
		return -EFAULT;

	if (!kbuf) {
		tmpbuf = vmalloc(count);
		if (!tmpbuf)
			return -ENOMEM;

		if (copy_from_user(tmpbuf, ubuf, count)) {
			ret = -EFAULT;
			goto out;
		}
	}

	fpu_force_restore(fpu);
	ret = copy_uabi_from_kernel_to_xstate(fpu->fpstate, kbuf ?: tmpbuf, &target->thread.pkru);

out:
	vfree(tmpbuf);
	return ret;
}
