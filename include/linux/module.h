/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _LINUX_MODULE_H
#define _LINUX_MODULE_H

#include <linux/compiler.h>
#include <linux/elf.h>
#include <linux/error-injection.h>
#include <linux/init.h>
#include <linux/kobject.h>
#include <linux/moduleparam.h>

#define MODULE_NAME_LEN __MODULE_NAME_LEN

struct module;
struct notifier_block;

struct module_kobject;

#define module_init(x)	__initcall(x)
#define module_exit(x)	__exitcall(x);

#define __init_or_module	__init
#define __initdata_or_module	__initdata
#define __initconst_or_module	__initconst

#define MODULE_ALIAS(_alias)
#define MODULE_SOFTDEP(_softdep)
#define MODULE_WEAKDEP(_weakdep)
#define MODULE_FILE
#define MODULE_LICENSE(_license)
#define MODULE_AUTHOR(_author)
#define MODULE_DESCRIPTION(_description)
#define MODULE_DEVICE_TABLE(type, name)
#define MODULE_VERSION(_version)
#define MODULE_FIRMWARE(_firmware)
#define MODULE_IMPORT_NS(ns)
#define __MODULE_STRING(x) __stringify(x)

static inline struct module *__module_address(unsigned long addr)
{
	return NULL;
}

static inline struct module *__module_text_address(unsigned long addr)
{
	return NULL;
}

static inline bool is_module_address(unsigned long addr)
{
	return false;
}

static inline bool is_module_percpu_address(unsigned long addr)
{
	return false;
}

static inline bool __is_module_percpu_address(unsigned long addr,
					      unsigned long *can_addr)
{
	return false;
}

static inline bool is_module_text_address(unsigned long addr)
{
	return false;
}

static inline bool within_module_core(unsigned long addr,
				      const struct module *mod)
{
	return false;
}

static inline bool within_module_init(unsigned long addr,
				      const struct module *mod)
{
	return false;
}

static inline bool within_module(unsigned long addr, const struct module *mod)
{
	return false;
}

#define symbol_get(x) ({ extern typeof(x) x __attribute__((weak,visibility("hidden"))); &(x); })
#define symbol_put(x) do { } while (0)
#define symbol_put_addr(x) do { } while (0)
#define symbol_request(x) symbol_get(x)

static inline void __module_get(struct module *module)
{
}

static inline bool try_module_get(struct module *module)
{
	return true;
}

static inline void module_put(struct module *module)
{
}

#define module_name(mod) "kernel"

static inline int register_module_notifier(struct notifier_block *nb)
{
	return 0;
}

static inline int unregister_module_notifier(struct notifier_block *nb)
{
	return 0;
}

static inline void print_modules(void)
{
}

static inline bool module_requested_async_probing(struct module *module)
{
	return false;
}

static inline void *dereference_module_function_descriptor(struct module *mod,
							   void *ptr)
{
	return ptr;
}

static inline bool module_is_coming(struct module *mod)
{
	return false;
}

static inline void module_for_each_mod(int (*func)(struct module *mod,
						   void *data),
				       void *data)
{
}

static inline int module_kallsyms_on_each_symbol(const char *modname,
						 int (*fn)(void *, const char *,
							   unsigned long),
						 void *data)
{
	return -EOPNOTSUPP;
}

static inline int module_address_lookup(unsigned long addr,
					unsigned long *symbolsize,
					unsigned long *offset,
					char **modname,
					const unsigned char **modbuildid,
					char *namebuf)
{
	return 0;
}

static inline int lookup_module_symbol_name(unsigned long addr, char *symname)
{
	return -ERANGE;
}

static inline int module_get_kallsym(unsigned int symnum, unsigned long *value,
				     char *type, char *name,
				     char *module_name, int *exported)
{
	return -ERANGE;
}

static inline unsigned long module_kallsyms_lookup_name(const char *name)
{
	return 0;
}

static inline unsigned long find_kallsyms_symbol_value(struct module *mod,
						       const char *name)
{
	return 0;
}

static inline bool retpoline_module_ok(bool has_retpoline)
{
	return true;
}

static inline bool is_module_sig_enforced(void)
{
	return false;
}

static inline void set_module_sig_enforced(void)
{
}

static inline bool module_sig_ok(struct module *module)
{
	return true;
}

DEFINE_FREE(module_put, struct module *, if (_T) module_put(_T))

#endif /* _LINUX_MODULE_H */
