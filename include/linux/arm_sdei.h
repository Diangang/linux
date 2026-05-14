/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __LINUX_ARM_SDEI_H
#define __LINUX_ARM_SDEI_H

static inline int sdei_mask_local_cpu(void) { return 0; }
static inline int sdei_unmask_local_cpu(void) { return 0; }
static inline void sdei_handler_abort(void) { }

#endif /* __LINUX_ARM_SDEI_H */
