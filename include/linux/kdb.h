/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _KDB_H
#define _KDB_H

/*
 * Kernel Debugger Architecture Independent Global Headers
 *
 * Copyright (c) 2000-2007 Silicon Graphics, Inc.  All Rights Reserved.
 * Copyright (C) 2000 Stephane Eranian <eranian@hpl.hp.com>
 * Copyright (C) 2009 Jason Wessel <jason.wessel@windriver.com>
 */

#include <linux/list.h>
#include <linux/smp.h>

/* Shifted versions of the command enable bits are be used if the command
 * has no arguments (see kdb_check_flags). This allows commands, such as
 * go, to have different permissions depending upon whether it is called
 * with an argument.
 */
#define KDB_ENABLE_NO_ARGS_SHIFT 10

typedef enum {
	KDB_ENABLE_ALL = (1 << 0), /* Enable everything */
	KDB_ENABLE_MEM_READ = (1 << 1),
	KDB_ENABLE_MEM_WRITE = (1 << 2),
	KDB_ENABLE_REG_READ = (1 << 3),
	KDB_ENABLE_REG_WRITE = (1 << 4),
	KDB_ENABLE_INSPECT = (1 << 5),
	KDB_ENABLE_FLOW_CTRL = (1 << 6),
	KDB_ENABLE_SIGNAL = (1 << 7),
	KDB_ENABLE_REBOOT = (1 << 8),
	/* User exposed values stop here, all remaining flags are
	 * exclusively used to describe a commands behaviour.
	 */

	KDB_ENABLE_ALWAYS_SAFE = (1 << 9),
	KDB_ENABLE_MASK = (1 << KDB_ENABLE_NO_ARGS_SHIFT) - 1,

	KDB_ENABLE_ALL_NO_ARGS = KDB_ENABLE_ALL << KDB_ENABLE_NO_ARGS_SHIFT,
	KDB_ENABLE_MEM_READ_NO_ARGS = KDB_ENABLE_MEM_READ
				      << KDB_ENABLE_NO_ARGS_SHIFT,
	KDB_ENABLE_MEM_WRITE_NO_ARGS = KDB_ENABLE_MEM_WRITE
				       << KDB_ENABLE_NO_ARGS_SHIFT,
	KDB_ENABLE_REG_READ_NO_ARGS = KDB_ENABLE_REG_READ
				      << KDB_ENABLE_NO_ARGS_SHIFT,
	KDB_ENABLE_REG_WRITE_NO_ARGS = KDB_ENABLE_REG_WRITE
				       << KDB_ENABLE_NO_ARGS_SHIFT,
	KDB_ENABLE_INSPECT_NO_ARGS = KDB_ENABLE_INSPECT
				     << KDB_ENABLE_NO_ARGS_SHIFT,
	KDB_ENABLE_FLOW_CTRL_NO_ARGS = KDB_ENABLE_FLOW_CTRL
				       << KDB_ENABLE_NO_ARGS_SHIFT,
	KDB_ENABLE_SIGNAL_NO_ARGS = KDB_ENABLE_SIGNAL
				    << KDB_ENABLE_NO_ARGS_SHIFT,
	KDB_ENABLE_REBOOT_NO_ARGS = KDB_ENABLE_REBOOT
				    << KDB_ENABLE_NO_ARGS_SHIFT,
	KDB_ENABLE_ALWAYS_SAFE_NO_ARGS = KDB_ENABLE_ALWAYS_SAFE
					 << KDB_ENABLE_NO_ARGS_SHIFT,
	KDB_ENABLE_MASK_NO_ARGS = KDB_ENABLE_MASK << KDB_ENABLE_NO_ARGS_SHIFT,

	KDB_REPEAT_NO_ARGS = 0x40000000, /* Repeat the command w/o arguments */
	KDB_REPEAT_WITH_ARGS = 0x80000000, /* Repeat the command with args */
} kdb_cmdflags_t;

typedef int (*kdb_func_t)(int, const char **);

/* The KDB shell command table */
typedef struct _kdbtab {
	char    *name;			/* Command name */
	kdb_func_t func;		/* Function to execute command */
	char    *usage;			/* Usage String for this command */
	char    *help;			/* Help message for this command */
	short    minlen;		/* Minimum legal # cmd chars required */
	kdb_cmdflags_t flags;		/* Command behaviour flags */
	struct list_head list_node;	/* Command list */
} kdbtab_t;

static inline __printf(1, 2) int kdb_printf(const char *fmt, ...) { return 0; }
static inline void kdb_init(int level) {}
static inline int kdb_register(kdbtab_t *cmd) { return 0; }
static inline void kdb_unregister(kdbtab_t *cmd) {}

static inline bool kdb_printf_on_this_cpu(void) { return false; }

enum {
	KDB_NOT_INITIALIZED,
	KDB_INIT_EARLY,
	KDB_INIT_FULL,
};

extern int kdbgetintenv(const char *, int *);
extern int kdb_set(int, const char **);
int kdb_lsmod(int argc, const char **argv);

#endif	/* !_KDB_H */
