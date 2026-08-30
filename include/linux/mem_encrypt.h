/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * AMD Memory Encryption Support
 *
 * Copyright (C) 2016 Advanced Micro Devices, Inc.
 *
 * Author: Tom Lendacky <thomas.lendacky@amd.com>
 */

#ifndef __MEM_ENCRYPT_H__
#define __MEM_ENCRYPT_H__

#ifndef __ASSEMBLY__

#ifdef CONFIG_ARCH_HAS_MEM_ENCRYPT

#include <asm/mem_encrypt.h>

#endif	/* CONFIG_ARCH_HAS_MEM_ENCRYPT */

#define __sme_set(x)		(x)
#define __sme_clr(x)		(x)

/*
 * dma_addr_encrypted() and dma_addr_unencrypted() are for converting a given DMA
 * address to the respective type of addressing.
 *
 * dma_addr_canonical() is used to reverse any conversions for encrypted/decrypted
 * back to the canonical address.
 */
#ifndef dma_addr_encrypted
#define dma_addr_encrypted(x)		(x)
#endif

#ifndef dma_addr_unencrypted
#define dma_addr_unencrypted(x)		(x)
#endif

#ifndef dma_addr_canonical
#define dma_addr_canonical(x)		(x)
#endif

#endif	/* __ASSEMBLY__ */

#endif	/* __MEM_ENCRYPT_H__ */
