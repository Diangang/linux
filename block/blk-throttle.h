/* SPDX-License-Identifier: GPL-2.0 */
#ifndef BLK_THROTTLE_H
#define BLK_THROTTLE_H

struct bio;
struct gendisk;

static inline void blk_throtl_exit(struct gendisk *disk) { }
static inline bool blk_throtl_bio(struct bio *bio) { return false; }
static inline void blk_throtl_cancel_bios(struct gendisk *disk) { }

#endif
