/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __LINUX_MEMFD_H
#define __LINUX_MEMFD_H

#include <linux/file.h>

#define MEMFD_ANON_NAME "[memfd]"

static inline long memfd_fcntl(struct file *f, unsigned int c, unsigned int a)
{
	return -EINVAL;
}
static inline struct folio *memfd_alloc_folio(struct file *memfd, pgoff_t idx)
{
	return ERR_PTR(-EINVAL);
}
static inline int memfd_check_seals_mmap(struct file *file,
					 vm_flags_t *vm_flags_ptr)
{
	return 0;
}

static inline struct file *memfd_alloc_file(const char *name, unsigned int flags)
{
	return ERR_PTR(-EINVAL);
}

static inline int memfd_get_seals(struct file *file)
{
	return -EINVAL;
}

static inline int memfd_add_seals(struct file *file, unsigned int seals)
{
	return -EINVAL;
}

#endif /* __LINUX_MEMFD_H */
