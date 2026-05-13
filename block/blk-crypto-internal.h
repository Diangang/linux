/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright 2019 Google LLC
 */

#ifndef __LINUX_BLK_CRYPTO_INTERNAL_H
#define __LINUX_BLK_CRYPTO_INTERNAL_H

#include <linux/bio.h>
#include <linux/blk-mq.h>

/* Represents a crypto mode supported by blk-crypto  */
static inline int blk_crypto_sysfs_register(struct gendisk *disk)
{
	return 0;
}

static inline void blk_crypto_sysfs_unregister(struct gendisk *disk)
{
}

static inline bool bio_crypt_rq_ctx_compatible(struct request *rq,
					       struct bio *bio)
{
	return true;
}

static inline bool bio_crypt_ctx_front_mergeable(struct request *req,
						 struct bio *bio)
{
	return true;
}

static inline bool bio_crypt_ctx_back_mergeable(struct request *req,
						struct bio *bio)
{
	return true;
}

static inline bool bio_crypt_ctx_merge_rq(struct request *req,
					  struct request *next)
{
	return true;
}

static inline void blk_crypto_rq_set_defaults(struct request *rq) { }

static inline bool blk_crypto_rq_is_encrypted(struct request *rq)
{
	return false;
}

static inline bool blk_crypto_rq_has_keyslot(struct request *rq)
{
	return false;
}

static inline int blk_crypto_ioctl(struct block_device *bdev, unsigned int cmd,
				   void __user *argp)
{
	return -ENOTTY;
}

static inline bool blk_crypto_supported(struct bio *bio)
{
	return false;
}

static inline void bio_crypt_advance(struct bio *bio, unsigned int bytes)
{
}

static inline void bio_crypt_free_ctx(struct bio *bio)
{
}

static inline void bio_crypt_do_front_merge(struct request *rq,
					    struct bio *bio)
{
}

static inline blk_status_t blk_crypto_rq_get_keyslot(struct request *rq)
{
	return BLK_STS_OK;
}

static inline void blk_crypto_rq_put_keyslot(struct request *rq)
{
}

static inline void blk_crypto_free_request(struct request *rq)
{
}

/**
 * blk_crypto_rq_bio_prep - Prepare a request's crypt_ctx when its first bio
 *			    is inserted
 * @rq: The request to prepare
 * @bio: The first bio being inserted into the request
 * @gfp_mask: Memory allocation flags
 *
 * Return: 0 on success, -ENOMEM if out of memory.  -ENOMEM is only possible if
 *	   @gfp_mask doesn't include %__GFP_DIRECT_RECLAIM.
 */
static inline int blk_crypto_rq_bio_prep(struct request *rq, struct bio *bio,
					 gfp_t gfp_mask)
{
	return 0;
}

bool blk_crypto_fallback_bio_prep(struct bio *bio);

static inline int
blk_crypto_fallback_start_using_mode(enum blk_crypto_mode_num mode_num)
{
	pr_warn_once("crypto API fallback is disabled\n");
	return -ENOPKG;
}

static inline int
blk_crypto_fallback_evict_key(const struct blk_crypto_key *key)
{
	return 0;
}

#endif /* __LINUX_BLK_CRYPTO_INTERNAL_H */
