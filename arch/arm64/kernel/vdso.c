// SPDX-License-Identifier: GPL-2.0-only
/*
 * VDSO implementations.
 *
 * Copyright (C) 2012 ARM Limited
 *
 * Author: Will Deacon <will.deacon@arm.com>
 */

#include <linux/cache.h>
#include <linux/clocksource.h>
#include <linux/elf.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/gfp.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/signal.h>
#include <linux/slab.h>
#include <linux/vdso_datastore.h>
#include <linux/vmalloc.h>
#include <vdso/datapage.h>
#include <vdso/helpers.h>
#include <vdso/vsyscall.h>

#include <asm/cacheflush.h>
#include <asm/vdso.h>

enum vdso_abi {
	VDSO_ABI_AA64,
};

struct vdso_abi_info {
	const char *name;
	const char *vdso_code_start;
	const char *vdso_code_end;
	unsigned long vdso_pages;
	/* Code Mapping */
	struct vm_special_mapping *cm;
};

static struct vdso_abi_info vdso_info[] __ro_after_init = {
	[VDSO_ABI_AA64] = {
		.name = "vdso",
		.vdso_code_start = vdso_start,
		.vdso_code_end = vdso_end,
	},
};

static int vdso_mremap(const struct vm_special_mapping *sm,
		struct vm_area_struct *new_vma)
{
	current->mm->context.vdso = (void *)new_vma->vm_start;

	return 0;
}

static int __init __vdso_init(enum vdso_abi abi)
{
	int i;
	struct page **vdso_pagelist;
	unsigned long pfn;

	if (memcmp(vdso_info[abi].vdso_code_start, "\177ELF", 4)) {
		pr_err("vDSO is not a valid ELF object!\n");
		return -EINVAL;
	}

	vdso_info[abi].vdso_pages = (
			vdso_info[abi].vdso_code_end -
			vdso_info[abi].vdso_code_start) >>
			PAGE_SHIFT;

	vdso_pagelist = kzalloc_objs(struct page *, vdso_info[abi].vdso_pages);
	if (vdso_pagelist == NULL)
		return -ENOMEM;

	/* Grab the vDSO code pages. */
	pfn = sym_to_pfn(vdso_info[abi].vdso_code_start);

	for (i = 0; i < vdso_info[abi].vdso_pages; i++)
		vdso_pagelist[i] = pfn_to_page(pfn + i);

	vdso_info[abi].cm->pages = vdso_pagelist;

	return 0;
}

static int __setup_additional_pages(enum vdso_abi abi,
				    struct mm_struct *mm,
				    struct linux_binprm *bprm,
				    int uses_interp)
{
	unsigned long vdso_base, vdso_text_len, vdso_mapping_len;
	unsigned long gp_flags = 0;
	void *ret;

	BUILD_BUG_ON(VDSO_NR_PAGES != __VDSO_PAGES);

	vdso_text_len = vdso_info[abi].vdso_pages << PAGE_SHIFT;
	/* Be sure to map the data page */
	vdso_mapping_len = vdso_text_len + VDSO_NR_PAGES * PAGE_SIZE;

	vdso_base = get_unmapped_area(NULL, 0, vdso_mapping_len, 0, 0);
	if (IS_ERR_VALUE(vdso_base)) {
		ret = ERR_PTR(vdso_base);
		goto up_fail;
	}

	ret = vdso_install_vvar_mapping(mm, vdso_base);
	if (IS_ERR(ret))
		goto up_fail;

	if (system_supports_bti_kernel())
		gp_flags = VM_ARM64_BTI;

	vdso_base += VDSO_NR_PAGES * PAGE_SIZE;
	mm->context.vdso = (void *)vdso_base;
	ret = _install_special_mapping(mm, vdso_base, vdso_text_len,
				       VM_READ|VM_EXEC|gp_flags|
				       VM_MAYREAD|VM_MAYWRITE|VM_MAYEXEC|
				       VM_SEALED_SYSMAP,
				       vdso_info[abi].cm);
	if (IS_ERR(ret))
		goto up_fail;

	return 0;

up_fail:
	mm->context.vdso = NULL;
	return PTR_ERR(ret);
}

static struct vm_special_mapping aarch64_vdso_map __ro_after_init = {
	.name	= "[vdso]",
	.mremap = vdso_mremap,
};

static int __init vdso_init(void)
{
	vdso_info[VDSO_ABI_AA64].cm = &aarch64_vdso_map;

	return __vdso_init(VDSO_ABI_AA64);
}
arch_initcall(vdso_init);

int arch_setup_additional_pages(struct linux_binprm *bprm, int uses_interp)
{
	struct mm_struct *mm = current->mm;
	int ret;

	if (mmap_write_lock_killable(mm))
		return -EINTR;

	ret = __setup_additional_pages(VDSO_ABI_AA64, mm, bprm, uses_interp);
	mmap_write_unlock(mm);

	return ret;
}
