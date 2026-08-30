/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Authentication token and access key management
 *
 * Copyright (C) 2004, 2007 Red Hat, Inc. All Rights Reserved.
 * Written by David Howells (dhowells@redhat.com)
 *
 * See Documentation/security/keys/core.rst for information on keys/keyrings.
 */

#ifndef _LINUX_KEY_H
#define _LINUX_KEY_H

#include <linux/types.h>
#include <linux/list.h>
#include <linux/rbtree.h>
#include <linux/rcupdate.h>
#include <linux/sysctl.h>
#include <linux/rwsem.h>
#include <linux/atomic.h>
#include <linux/assoc_array.h>
#include <linux/refcount.h>
#include <linux/time64.h>

#ifdef __KERNEL__
#include <linux/uidgid.h>

/* key handle serial number */
typedef int32_t key_serial_t;

/* key handle permissions mask */
typedef uint32_t key_perm_t;

struct key;

#define key_validate(k)			0
#define key_serial(k)			0
#define key_get(k) 			({ NULL; })
#define key_revoke(k)			do { } while(0)
#define key_invalidate(k)		do { } while(0)
#define key_put(k)			do { } while(0)
#define key_ref_put(k)			do { } while(0)
#define make_key_ref(k, p)		NULL
#define key_ref_to_ptr(k)		NULL
#define is_key_possessed(k)		0
#define key_fsuid_changed(c)		do { } while(0)
#define key_fsgid_changed(c)		do { } while(0)
#define key_init()			do { } while(0)
#define key_free_user_ns(ns)		do { } while(0)
#define key_remove_domain(d)		do { } while(0)
#define key_lookup(k)			NULL

#endif /* __KERNEL__ */
#endif /* _LINUX_KEY_H */
