/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2013 - 2017 Linaro, Ltd.
 * Copyright (C) 2013, 2014 Red Hat, Inc.
 */

#include <linux/pe.h>
#include <linux/sizes.h>

	.macro	efi_signature_nop
	/*
	 * Bootloaders may inspect the opcode at the start of the kernel
	 * image to decide if the kernel is capable of booting via UEFI.
	 * So put an ordinary NOP here, not the "MZ.." pseudo-nop above.
	 */
	nop
	.endm

	.macro	__EFI_PE_HEADER
	.set	.Lpe_header_offset, 0x0
	.endm
