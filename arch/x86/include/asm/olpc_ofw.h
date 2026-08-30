/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_X86_OLPC_OFW_H
#define _ASM_X86_OLPC_OFW_H

/* index into the page table containing the entry OFW occupies */
#define OLPC_OFW_PDE_NR 1022

#define OLPC_OFW_SIG 0x2057464F	/* aka "OFW " */

static inline void olpc_ofw_detect(void) { }
static inline void setup_olpc_ofw_pgd(void) { }
static inline void olpc_dt_build_devicetree(void) { }

#endif /* _ASM_X86_OLPC_OFW_H */
