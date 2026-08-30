/* SPDX-License-Identifier: GPL-2.0 */
/* Rewritten and vastly simplified by Rusty Russell for in-kernel
 * module loader:
 *   Copyright 2002 Rusty Russell <rusty@rustcorp.com.au> IBM Corporation
 */
#ifndef _LINUX_KALLSYMS_H
#define _LINUX_KALLSYMS_H

#include <linux/errno.h>
#include <linux/buildid.h>
#include <linux/kernel.h>
#include <linux/stddef.h>
#include <linux/mm.h>
#include <linux/module.h>

#include <asm/sections.h>

#define KSYM_NAME_LEN 512
#define KSYM_SYMBOL_LEN (sizeof("%s+%#lx/%#lx [%s %s]") + \
			(KSYM_NAME_LEN - 1) + \
			2*(BITS_PER_LONG*3/10) + (MODULE_NAME_LEN - 1) + \
			(BUILD_ID_SIZE_MAX * 2) + 1)

struct cred;
struct module;

static inline int is_kernel_text(unsigned long addr)
{
	if (__is_kernel_text(addr))
		return 1;
	return in_gate_area_no_mm(addr);
}

static inline int is_kernel(unsigned long addr)
{
	if (__is_kernel(addr))
		return 1;
	return in_gate_area_no_mm(addr);
}

static inline int is_ksym_addr(unsigned long addr)
{
	return is_kernel_text(addr) || is_kernel_inittext(addr);
}

static inline void *dereference_symbol_descriptor(void *ptr)
{
	return ptr;
}

/* How and when do we show kallsyms values? */
extern bool kallsyms_show_value(const struct cred *cred);


static inline unsigned long kallsyms_lookup_name(const char *name)
{
	return 0;
}

static inline int kallsyms_lookup_size_offset(unsigned long addr,
					      unsigned long *symbolsize,
					      unsigned long *offset)
{
	return 0;
}

static inline const char *kallsyms_lookup(unsigned long addr,
					  unsigned long *symbolsize,
					  unsigned long *offset,
					  char **modname, char *namebuf)
{
	return NULL;
}

static inline int sprint_symbol(char *buffer, unsigned long addr)
{
	*buffer = '\0';
	return 0;
}

static inline int sprint_symbol_build_id(char *buffer, unsigned long address)
{
	*buffer = '\0';
	return 0;
}

static inline int sprint_symbol_no_offset(char *buffer, unsigned long addr)
{
	*buffer = '\0';
	return 0;
}

static inline int sprint_backtrace(char *buffer, unsigned long addr)
{
	*buffer = '\0';
	return 0;
}

static inline int sprint_backtrace_build_id(char *buffer, unsigned long addr)
{
	*buffer = '\0';
	return 0;
}

static inline int lookup_symbol_name(unsigned long addr, char *symname)
{
	return -ERANGE;
}

static inline int kallsyms_on_each_symbol(int (*fn)(void *, const char *, unsigned long),
					  void *data)
{
	return -EOPNOTSUPP;
}

static inline int kallsyms_on_each_match_symbol(int (*fn)(void *, unsigned long),
						const char *name, void *data)
{
	return -EOPNOTSUPP;
}

static inline void print_ip_sym(const char *loglvl, unsigned long ip)
{
	printk("%s[<%px>] %pS\n", loglvl, (void *) ip, (void *) ip);
}

#endif /*_LINUX_KALLSYMS_H*/
