/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright 2019 Google LLC
 */

#ifndef __LINUX_BLK_CRYPTO_H
#define __LINUX_BLK_CRYPTO_H

#include <linux/minmax.h>
#include <linux/types.h>
#include <uapi/linux/blk-crypto.h>

enum blk_crypto_mode_num {
	BLK_ENCRYPTION_MODE_INVALID,
	BLK_ENCRYPTION_MODE_AES_256_XTS,
	BLK_ENCRYPTION_MODE_AES_128_CBC_ESSIV,
	BLK_ENCRYPTION_MODE_ADIANTUM,
	BLK_ENCRYPTION_MODE_SM4_XTS,
	BLK_ENCRYPTION_MODE_MAX,
};

/*
 * Supported types of keys.  Must be bitflags due to their use in
 * blk_crypto_profile::key_types_supported.
 */
enum blk_crypto_key_type {
	/*
	 * Raw keys (i.e. "software keys").  These keys are simply kept in raw,
	 * plaintext form in kernel memory.
	 */
	BLK_CRYPTO_KEY_TYPE_RAW = 0x1,

	/*
	 * Hardware-wrapped keys.  These keys are only present in kernel memory
	 * in ephemerally-wrapped form, and they can only be unwrapped by
	 * dedicated hardware.  For details, see the "Hardware-wrapped keys"
	 * section of Documentation/block/inline-encryption.rst.
	 */
	BLK_CRYPTO_KEY_TYPE_HW_WRAPPED = 0x2,
};

/*
 * Currently the maximum raw key size is 64 bytes, as that is the key size of
 * BLK_ENCRYPTION_MODE_AES_256_XTS which takes the longest key.
 *
 * The maximum hardware-wrapped key size depends on the hardware's key wrapping
 * algorithm, which is a hardware implementation detail, so it isn't precisely
 * specified.  But currently 128 bytes is plenty in practice.  Implementations
 * are recommended to wrap a 32-byte key for the hardware KDF with AES-256-GCM,
 * which should result in a size closer to 64 bytes than 128.
 *
 * Both of these values can trivially be increased if ever needed.
 */
#define BLK_CRYPTO_MAX_RAW_KEY_SIZE		64
#define BLK_CRYPTO_MAX_HW_WRAPPED_KEY_SIZE	128

#define BLK_CRYPTO_MAX_ANY_KEY_SIZE \
	MAX(BLK_CRYPTO_MAX_RAW_KEY_SIZE, BLK_CRYPTO_MAX_HW_WRAPPED_KEY_SIZE)

/*
 * Size of the "software secret" which can be derived from a hardware-wrapped
 * key.  This is currently always 32 bytes.  Note, the choice of 32 bytes
 * assumes that the software secret is only used directly for algorithms that
 * don't require more than a 256-bit key to get the desired security strength.
 * If it were to be used e.g. directly as an AES-256-XTS key, then this would
 * need to be increased (which is possible if hardware supports it, but care
 * would need to be taken to avoid breaking users who need exactly 32 bytes).
 */
#define BLK_CRYPTO_SW_SECRET_SIZE	32

/**
 * struct blk_crypto_config - an inline encryption key's crypto configuration
 * @crypto_mode: encryption algorithm this key is for
 * @data_unit_size: the data unit size for all encryption/decryptions with this
 *	key.  This is the size in bytes of each individual plaintext and
 *	ciphertext.  This is always a power of 2.  It might be e.g. the
 *	filesystem block size or the disk sector size.
 * @dun_bytes: the maximum number of bytes of DUN used when using this key
 * @key_type: the type of this key -- either raw or hardware-wrapped
 */
struct blk_crypto_config {
	enum blk_crypto_mode_num crypto_mode;
	unsigned int data_unit_size;
	unsigned int dun_bytes;
	enum blk_crypto_key_type key_type;
};

/**
 * struct blk_crypto_key - an inline encryption key
 * @crypto_cfg: the crypto mode, data unit size, key type, and other
 *		characteristics of this key and how it will be used
 * @data_unit_size_bits: log2 of data_unit_size
 * @size: size of this key in bytes.  The size of a raw key is fixed for a given
 *	  crypto mode, but the size of a hardware-wrapped key can vary.
 * @bytes: the bytes of this key.  Only the first @size bytes are significant.
 *
 * A blk_crypto_key is immutable once created, and many bios can reference it at
 * the same time.  It must not be freed until all bios using it have completed
 * and it has been evicted from all devices on which it may have been used.
 */
struct blk_crypto_key {
	struct blk_crypto_config crypto_cfg;
	unsigned int data_unit_size_bits;
	unsigned int size;
	u8 bytes[BLK_CRYPTO_MAX_ANY_KEY_SIZE];
};

#define BLK_CRYPTO_MAX_IV_SIZE		32
#define BLK_CRYPTO_DUN_ARRAY_SIZE	(BLK_CRYPTO_MAX_IV_SIZE / sizeof(u64))

#include <linux/blk_types.h>
#include <linux/blkdev.h>

static inline bool bio_has_crypt_ctx(struct bio *bio)
{
	return false;
}

static inline struct bio_crypt_ctx *bio_crypt_ctx(struct bio *bio)
{
	return NULL;
}

/**
 * blk_crypto_submit_bio - Submit a bio that may have a crypto context
 * @bio: bio to submit
 *
 * If @bio has no crypto context, or the crypt context attached to @bio is
 * supported by the underlying device's inline encryption hardware, just submit
 * @bio.
 *
 * Otherwise, try to perform en/decryption for this bio by falling back to the
 * kernel crypto API. For encryption this means submitting newly allocated
 * bios for the encrypted payload while keeping back the source bio until they
 * complete, while for reads the decryption happens in-place by a hooked in
 * completion handler.
 */
static inline void blk_crypto_submit_bio(struct bio *bio)
{
	submit_bio(bio);
}

/**
 * bio_crypt_clone - clone bio encryption context
 * @dst: destination bio
 * @src: source bio
 * @gfp_mask: memory allocation flags
 *
 * If @src has an encryption context, clone it to @dst.
 *
 * Return: 0 on success, -ENOMEM if out of memory.  -ENOMEM is only possible if
 *	   @gfp_mask doesn't include %__GFP_DIRECT_RECLAIM.
 */
static inline int bio_crypt_clone(struct bio *dst, struct bio *src,
				  gfp_t gfp_mask)
{
	return 0;
}

#endif /* __LINUX_BLK_CRYPTO_H */
