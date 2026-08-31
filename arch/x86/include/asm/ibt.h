/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_X86_IBT_H
#define _ASM_X86_IBT_H

#define HAS_KERNEL_IBT	0

#ifndef __ASSEMBLER__

#define ASM_ENDBR
#define IBT_NOSEAL(name)

#define __noendbr

#else /* __ASSEMBLER__ */

#define ENDBR

#endif /* __ASSEMBLER__ */


#define ENDBR_INSN_SIZE		(4*HAS_KERNEL_IBT)

#endif /* _ASM_X86_IBT_H */
