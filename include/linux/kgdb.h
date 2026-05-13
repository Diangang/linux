/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _KGDB_H_
#define _KGDB_H_

#define in_dbg_master() (0)
#define dbg_late_init()

static inline void kgdb_panic(const char *msg) { }
static inline void kgdb_free_init_mem(void) { }
static inline int kgdb_nmicallback(int cpu, void *regs) { return 1; }

#endif /* _KGDB_H_ */
