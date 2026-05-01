// SPDX-License-Identifier: GPL-2.0
#include <linux/ucs2_string.h>
#include <linux/module.h>
#include <linux/nls.h>
#include <asm/byteorder.h>

/* Return the number of unicode characters in data */
unsigned long
ucs2_strnlen(const ucs2_char_t *s, size_t maxlength)
{
        unsigned long length = 0;

        while (*s++ != 0 && length < maxlength)
                length++;
        return length;
}
EXPORT_SYMBOL(ucs2_strnlen);

unsigned long
ucs2_strlen(const ucs2_char_t *s)
{
        return ucs2_strnlen(s, ~0UL);
}
EXPORT_SYMBOL(ucs2_strlen);

/*
 * Return the number of bytes is the length of this string
 * Note: this is NOT the same as the number of unicode characters
 */
unsigned long
ucs2_strsize(const ucs2_char_t *data, unsigned long maxlength)
{
        return ucs2_strnlen(data, maxlength/sizeof(ucs2_char_t)) * sizeof(ucs2_char_t);
}
EXPORT_SYMBOL(ucs2_strsize);

/**
 * ucs2_strscpy() - Copy a UCS2 string into a sized buffer.
 *
 * @dst: Pointer to the destination buffer where to copy the string to.
 * @src: Pointer to the source buffer where to copy the string from.
 * @count: Size of the destination buffer, in UCS2 (16-bit) characters.
 *
 * Like strscpy(), only for UCS2 strings.
 *
 * Copy the source string @src, or as much of it as fits, into the destination
 * buffer @dst. The behavior is undefined if the string buffers overlap. The
 * destination buffer @dst is always NUL-terminated, unless it's zero-sized.
 *
 * Return: The number of characters copied into @dst (excluding the trailing
 * %NUL terminator) or -E2BIG if @count is 0 or @src was truncated due to the
 * destination buffer being too small.
 */
ssize_t ucs2_strscpy(ucs2_char_t *dst, const ucs2_char_t *src, size_t count)
{
	long res;

	/*
	 * Ensure that we have a valid amount of space. We need to store at
	 * least one NUL-character.
	 */
	if (count == 0 || WARN_ON_ONCE(count > INT_MAX / sizeof(*dst)))
		return -E2BIG;

	/*
	 * Copy at most 'count' characters, return early if we find a
	 * NUL-terminator.
	 */
	for (res = 0; res < count; res++) {
		ucs2_char_t c;

		c = src[res];
		dst[res] = c;

		if (!c)
			return res;
	}

	/*
	 * The loop above terminated without finding a NUL-terminator,
	 * exceeding the 'count': Enforce proper NUL-termination and return
	 * error.
	 */
	dst[count - 1] = 0;
	return -E2BIG;
}
EXPORT_SYMBOL(ucs2_strscpy);

int
ucs2_strncmp(const ucs2_char_t *a, const ucs2_char_t *b, size_t len)
{
        while (1) {
                if (len == 0)
                        return 0;
                if (*a < *b)
                        return -1;
                if (*a > *b)
                        return 1;
                if (*a == 0) /* implies *b == 0 */
                        return 0;
                a++;
                b++;
                len--;
        }
}
EXPORT_SYMBOL(ucs2_strncmp);

unsigned long
ucs2_utf8size(const ucs2_char_t *src)
{
	unsigned long i;
	unsigned long j = 0;

	for (i = 0; src[i]; i++) {
		u16 c = src[i];

		if (c >= 0x800)
			j += 3;
		else if (c >= 0x80)
			j += 2;
		else
			j += 1;
	}

	return j;
}
EXPORT_SYMBOL(ucs2_utf8size);

/*
 * copy at most maxlength bytes of whole utf8 characters to dest from the
 * ucs2 string src.
 *
 * The return value is the number of characters copied, not including the
 * final NUL character.
 */
unsigned long
ucs2_as_utf8(u8 *dest, const ucs2_char_t *src, unsigned long maxlength)
{
	unsigned int i;
	unsigned long j = 0;
	unsigned long limit = ucs2_strnlen(src, maxlength);

	for (i = 0; maxlength && i < limit; i++) {
		u16 c = src[i];

		if (c >= 0x800) {
			if (maxlength < 3)
				break;
			maxlength -= 3;
			dest[j++] = 0xe0 | (c & 0xf000) >> 12;
			dest[j++] = 0x80 | (c & 0x0fc0) >> 6;
			dest[j++] = 0x80 | (c & 0x003f);
		} else if (c >= 0x80) {
			if (maxlength < 2)
				break;
			maxlength -= 2;
			dest[j++] = 0xc0 | (c & 0x7c0) >> 6;
			dest[j++] = 0x80 | (c & 0x03f);
		} else {
			maxlength -= 1;
			dest[j++] = c & 0x7f;
		}
	}
	if (maxlength)
		dest[j] = '\0';
	return j;
}
EXPORT_SYMBOL(ucs2_as_utf8);

struct utf8_table {
	int     cmask;
	int     cval;
	int     shift;
	long    lmask;
	long    lval;
};

static const struct utf8_table utf8_table[] = {
	{0x80,  0x00,   0*6,    0x7F,           0},
	{0xE0,  0xC0,   1*6,    0x7FF,          0x80},
	{0xF0,  0xE0,   2*6,    0xFFFF,         0x800},
	{0xF8,  0xF0,   3*6,    0x1FFFFF,       0x10000},
	{0xFC,  0xF8,   4*6,    0x3FFFFFF,      0x200000},
	{0xFE,  0xFC,   5*6,    0x7FFFFFFF,     0x4000000},
	{0}
};

#define UNICODE_MAX	0x0010ffff
#define PLANE_SIZE	0x00010000

#define SURROGATE_MASK	0xfffff800
#define SURROGATE_PAIR	0x0000d800
#define SURROGATE_LOW	0x00000400
#define SURROGATE_BITS	0x000003ff

int utf8_to_utf32(const u8 *s, int inlen, unicode_t *pu)
{
	unsigned long l;
	int c0, c, nc;
	const struct utf8_table *t;

	nc = 0;
	c0 = *s;
	l = c0;
	for (t = utf8_table; t->cmask; t++) {
		nc++;
		if ((c0 & t->cmask) == t->cval) {
			l &= t->lmask;
			if (l < t->lval || l > UNICODE_MAX ||
			    (l & SURROGATE_MASK) == SURROGATE_PAIR)
				return -EILSEQ;

			*pu = (unicode_t)l;
			return nc;
		}
		if (inlen <= nc)
			return -EOVERFLOW;

		s++;
		c = (*s ^ 0x80) & 0xFF;
		if (c & 0xC0)
			return -EILSEQ;

		l = (l << 6) | c;
	}
	return -EILSEQ;
}
EXPORT_SYMBOL(utf8_to_utf32);

int utf32_to_utf8(unicode_t u, u8 *s, int maxout)
{
	unsigned long l;
	int c, nc;
	const struct utf8_table *t;

	if (!s)
		return 0;

	l = u;
	if (l > UNICODE_MAX || (l & SURROGATE_MASK) == SURROGATE_PAIR)
		return -EILSEQ;

	nc = 0;
	for (t = utf8_table; t->cmask && maxout; t++, maxout--) {
		nc++;
		if (l <= t->lmask) {
			c = t->shift;
			*s = (u8)(t->cval | (l >> c));
			while (c > 0) {
				c -= 6;
				s++;
				*s = (u8)(0x80 | ((l >> c) & 0x3F));
			}
			return nc;
		}
	}
	return -EOVERFLOW;
}
EXPORT_SYMBOL(utf32_to_utf8);

static inline void put_utf16(wchar_t *s, unsigned int c,
			     enum utf16_endian endian)
{
	switch (endian) {
	default:
		*s = (wchar_t)c;
		break;
	case UTF16_LITTLE_ENDIAN:
		*s = __cpu_to_le16(c);
		break;
	case UTF16_BIG_ENDIAN:
		*s = __cpu_to_be16(c);
		break;
	}
}

int utf8s_to_utf16s(const u8 *s, int inlen, enum utf16_endian endian,
		    wchar_t *pwcs, int maxout)
{
	u16 *op = pwcs;
	int size;
	unicode_t u;

	while (inlen > 0 && maxout > 0 && *s) {
		if (*s & 0x80) {
			size = utf8_to_utf32(s, inlen, &u);
			if (size < 0)
				return -EINVAL;
			s += size;
			inlen -= size;

			if (u >= PLANE_SIZE) {
				if (maxout < 2)
					break;
				u -= PLANE_SIZE;
				put_utf16(op++, SURROGATE_PAIR |
					  ((u >> 10) & SURROGATE_BITS),
					  endian);
				put_utf16(op++, SURROGATE_PAIR |
					  SURROGATE_LOW |
					  (u & SURROGATE_BITS),
					  endian);
				maxout -= 2;
			} else {
				put_utf16(op++, u, endian);
				maxout--;
			}
		} else {
			put_utf16(op++, *s++, endian);
			inlen--;
			maxout--;
		}
	}
	return op - pwcs;
}
EXPORT_SYMBOL(utf8s_to_utf16s);

static inline unsigned long get_utf16(unsigned int c, enum utf16_endian endian)
{
	switch (endian) {
	default:
		return c;
	case UTF16_LITTLE_ENDIAN:
		return __le16_to_cpu(c);
	case UTF16_BIG_ENDIAN:
		return __be16_to_cpu(c);
	}
}

int utf16s_to_utf8s(const wchar_t *pwcs, int inlen, enum utf16_endian endian,
		    u8 *s, int maxout)
{
	u8 *op = s;
	int size;
	unsigned long u, v;

	while (inlen > 0 && maxout > 0) {
		u = get_utf16(*pwcs, endian);
		if (!u)
			break;
		pwcs++;
		inlen--;
		if (u > 0x7f) {
			if ((u & SURROGATE_MASK) == SURROGATE_PAIR) {
				if (u & SURROGATE_LOW)
					continue;
				if (inlen <= 0)
					break;
				v = get_utf16(*pwcs, endian);
				if ((v & SURROGATE_MASK) != SURROGATE_PAIR ||
				    !(v & SURROGATE_LOW))
					continue;
				u = PLANE_SIZE + ((u & SURROGATE_BITS) << 10)
					+ (v & SURROGATE_BITS);
				pwcs++;
				inlen--;
			}
			size = utf32_to_utf8(u, op, maxout);
			if (size < 0) {
				if (size == -EILSEQ)
					continue;
				break;
			}
			op += size;
			maxout -= size;
		} else {
			*op++ = (u8)u;
			maxout--;
		}
	}
	return op - s;
}
EXPORT_SYMBOL(utf16s_to_utf8s);

MODULE_DESCRIPTION("UCS2 string handling");
MODULE_LICENSE("GPL v2");
