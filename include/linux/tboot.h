/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * tboot.h: shared data structure with tboot and kernel and functions
 *          used by kernel for runtime support of Intel(R) Trusted
 *          Execution Technology
 *
 * Copyright (c) 2006-2009, Intel Corporation
 */

#ifndef _LINUX_TBOOT_H
#define _LINUX_TBOOT_H

/* these must have the values from 0-5 in this order */
enum {
	TB_SHUTDOWN_REBOOT = 0,
	TB_SHUTDOWN_S5,
	TB_SHUTDOWN_S4,
	TB_SHUTDOWN_S3,
	TB_SHUTDOWN_HALT,
	TB_SHUTDOWN_WFS
};


#define tboot_enabled()			0
#define tboot_probe()			do { } while (0)
#define tboot_shutdown(shutdown_type)	do { } while (0)
#define tboot_sleep(sleep_state, pm1a_control, pm1b_control)	\
					do { } while (0)
#define tboot_get_dmar_table(dmar_tbl)	(dmar_tbl)


#endif /* _LINUX_TBOOT_H */
