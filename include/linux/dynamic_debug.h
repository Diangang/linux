/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _DYNAMIC_DEBUG_H
#define _DYNAMIC_DEBUG_H

#if defined(CONFIG_JUMP_LABEL)
#include <linux/jump_label.h>
#endif

#include <linux/build_bug.h>

/*
 * An instance of this structure is created in a special
 * ELF section at every dynamic debug callsite.  At runtime,
 * the special section is treated as an array of these.
 */
struct _ddebug {
	/*
	 * These fields are used to drive the user interface
	 * for selecting and displaying debug callsites.
	 */
	const char *modname;
	const char *function;
	const char *filename;
	const char *format;
	unsigned int lineno:18;
#define CLS_BITS 6
	unsigned int class_id:CLS_BITS;
#define _DPRINTK_CLASS_DFLT		((1 << CLS_BITS) - 1)
	/*
	 * The flags field controls the behaviour at the callsite.
	 * The bits here are changed dynamically when the user
	 * writes commands to <debugfs>/dynamic_debug/control
	 */
#define _DPRINTK_FLAGS_NONE	0
#define _DPRINTK_FLAGS_PRINT	(1<<0) /* printk() a message using the format */
#define _DPRINTK_FLAGS_INCL_MODNAME	(1<<1)
#define _DPRINTK_FLAGS_INCL_FUNCNAME	(1<<2)
#define _DPRINTK_FLAGS_INCL_LINENO	(1<<3)
#define _DPRINTK_FLAGS_INCL_TID		(1<<4)
#define _DPRINTK_FLAGS_INCL_SOURCENAME	(1<<5)
#define _DPRINTK_FLAGS_INCL_STACK	(1<<6)

#define _DPRINTK_FLAGS_INCL_ANY		\
	(_DPRINTK_FLAGS_INCL_MODNAME | _DPRINTK_FLAGS_INCL_FUNCNAME |\
	 _DPRINTK_FLAGS_INCL_LINENO  | _DPRINTK_FLAGS_INCL_TID |\
	 _DPRINTK_FLAGS_INCL_SOURCENAME | _DPRINTK_FLAGS_INCL_STACK)

#if defined DEBUG
#define _DPRINTK_FLAGS_DEFAULT _DPRINTK_FLAGS_PRINT
#else
#define _DPRINTK_FLAGS_DEFAULT 0
#endif
	unsigned int flags:8;
#ifdef CONFIG_JUMP_LABEL
	union {
		struct static_key_true dd_key_true;
		struct static_key_false dd_key_false;
	} key;
#endif
} __attribute__((aligned(8)));

enum class_map_type {
	DD_CLASS_TYPE_DISJOINT_BITS,
	/**
	 * DD_CLASS_TYPE_DISJOINT_BITS: classes are independent, one per bit.
	 * expecting hex input. Built for drm.debug, basis for other types.
	 */
	DD_CLASS_TYPE_LEVEL_NUM,
	/**
	 * DD_CLASS_TYPE_LEVEL_NUM: input is numeric level, 0-N.
	 * N turns on just bits N-1 .. 0, so N=0 turns all bits off.
	 */
	DD_CLASS_TYPE_DISJOINT_NAMES,
	/**
	 * DD_CLASS_TYPE_DISJOINT_NAMES: input is a CSV of [+-]CLASS_NAMES,
	 * classes are independent, like _DISJOINT_BITS.
	 */
	DD_CLASS_TYPE_LEVEL_NAMES,
	/**
	 * DD_CLASS_TYPE_LEVEL_NAMES: input is a CSV of [+-]CLASS_NAMES,
	 * intended for names like: INFO,DEBUG,TRACE, with a module prefix
	 * avoid EMERG,ALERT,CRIT,ERR,WARNING: they're not debug
	 */
};

struct ddebug_class_map {
	struct list_head link;
	struct module *mod;
	const char *mod_name;	/* needed for builtins */
	const char **class_names;
	const int length;
	const int base;		/* index of 1st .class_id, allows split/shared space */
	enum class_map_type map_type;
};

/**
 * DECLARE_DYNDBG_CLASSMAP - declare classnames known by a module
 * @_var:   a struct ddebug_class_map, passed to module_param_cb
 * @_type:  enum class_map_type, chooses bits/verbose, numeric/symbolic
 * @_base:  offset of 1st class-name. splits .class_id space
 * @classes: class-names used to control class'd prdbgs
 */
#define DECLARE_DYNDBG_CLASSMAP(_var, _maptype, _base, ...)		\
	static const char *_var##_classnames[] = { __VA_ARGS__ };	\
	static struct ddebug_class_map __aligned(8) __used		\
		__section("__dyndbg_classes") _var = {			\
		.mod = THIS_MODULE,					\
		.mod_name = KBUILD_MODNAME,				\
		.base = _base,						\
		.map_type = _maptype,					\
		.length = NUM_TYPE_ARGS(char*, __VA_ARGS__),		\
		.class_names = _var##_classnames,			\
	}
#define NUM_TYPE_ARGS(eltype, ...)				\
        (sizeof((eltype[]){__VA_ARGS__}) / sizeof(eltype))

/* encapsulate linker provided built-in (or module) dyndbg data */
struct _ddebug_info {
	struct _ddebug *descs;
	struct ddebug_class_map *classes;
	unsigned int num_descs;
	unsigned int num_classes;
};

struct ddebug_class_param {
	union {
		unsigned long *bits;
		unsigned int *lvl;
	};
	char flags[8];
	const struct ddebug_class_map *map;
};

/*
 * pr_debug() and friends are globally enabled or modules have selectively
 * enabled them.
 */

#include <linux/string.h>
#include <linux/errno.h>
#include <linux/printk.h>

#define DEFINE_DYNAMIC_DEBUG_METADATA(name, fmt)
#define DYNAMIC_DEBUG_BRANCH(descriptor) false

#define dynamic_pr_debug(fmt, ...)					\
	no_printk(KERN_DEBUG pr_fmt(fmt), ##__VA_ARGS__)
#define dynamic_dev_dbg(dev, fmt, ...)					\
	dev_no_printk(KERN_DEBUG, dev, fmt, ##__VA_ARGS__)
#define dynamic_hex_dump(prefix_str, prefix_type, rowsize,		\
			 groupsize, buf, len, ascii)			\
	do { if (0)							\
		print_hex_dump(KERN_DEBUG, prefix_str, prefix_type,	\
				rowsize, groupsize, buf, len, ascii);	\
	} while (0)




static inline int ddebug_dyndbg_module_param_cb(char *param, char *val,
						const char *modname)
{
	if (!strcmp(param, "dyndbg")) {
		/* avoid pr_warn(), which wants pr_fmt() fully defined */
		printk(KERN_WARNING "dyndbg param is supported only in "
			"CONFIG_DYNAMIC_DEBUG builds\n");
		return 0; /* allow and ignore */
	}
	return -EINVAL;
}

struct kernel_param;
static inline int param_set_dyndbg_classes(const char *instr, const struct kernel_param *kp)
{ return 0; }
static inline int param_get_dyndbg_classes(char *buffer, const struct kernel_param *kp)
{ return 0; }



extern const struct kernel_param_ops param_ops_dyndbg_classes;

#endif /* _DYNAMIC_DEBUG_H */
