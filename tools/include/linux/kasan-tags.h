/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_KASAN_TAGS_H
#define _LINUX_KASAN_TAGS_H

#define KASAN_TAG_KERNEL	0xFF /* native kernel pointers tag */
#define KASAN_TAG_INVALID	0xFE /* inaccessible memory tag */
#define KASAN_TAG_MAX		0xFD /* maximum value for random tags */

#define KASAN_TAG_MIN		0x00 /* minimum value for random tags */

#endif /* LINUX_KASAN_TAGS_H */
