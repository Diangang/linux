/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
#ifndef _LINUX_SECRETMEM_H
#define _LINUX_SECRETMEM_H


static inline bool vma_is_secretmem(struct vm_area_struct *vma)
{
	return false;
}

static inline bool secretmem_mapping(struct address_space *mapping)
{
	return false;
}

static inline bool secretmem_active(void)
{
	return false;
}


#endif /* _LINUX_SECRETMEM_H */
