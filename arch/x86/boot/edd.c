// SPDX-License-Identifier: GPL-2.0-only
/* -*- linux-c -*- ------------------------------------------------------- *
 *
 *   Copyright (C) 1991, 1992 Linus Torvalds
 *   Copyright 2007 rPath, Inc. - All Rights Reserved
 *   Copyright 2009 Intel Corporation; author H. Peter Anvin
 *
 * ----------------------------------------------------------------------- */

/*
 * Get EDD BIOS disk information
 */

#include "boot.h"
#include <linux/edd.h>
#include "string.h"
