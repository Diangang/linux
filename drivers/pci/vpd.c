// SPDX-License-Identifier: GPL-2.0
/*
 * PCI VPD support
 *
 * Copyright (C) 2010 Broadcom Corporation.
 */

#include <linux/pci.h>
#include <linux/delay.h>
#include <linux/export.h>
#include <linux/sched/signal.h>
#include <linux/unaligned.h>
#include "pci.h"

#define PCI_VPD_LRDT_TAG_SIZE		3
#define PCI_VPD_SRDT_LEN_MASK		0x07
#define PCI_VPD_SRDT_TAG_SIZE		1
#define PCI_VPD_STIN_END		0x0f
#define PCI_VPD_INFO_FLD_HDR_SIZE	3

static u16 pci_vpd_lrdt_size(const u8 *lrdt)
{
	return get_unaligned_le16(lrdt + 1);
}

static u8 pci_vpd_srdt_tag(const u8 *srdt)
{
	return *srdt >> 3;
}

static u8 pci_vpd_srdt_size(const u8 *srdt)
{
	return *srdt & PCI_VPD_SRDT_LEN_MASK;
}

static u8 pci_vpd_info_field_size(const u8 *info_field)
{
	return info_field[2];
}

/* VPD access through PCI 2.2+ VPD capability */

static struct pci_dev *pci_get_func0_dev(struct pci_dev *dev)
{
	return pci_get_slot(dev->bus, PCI_DEVFN(PCI_SLOT(dev->devfn), 0));
}

#define PCI_VPD_MAX_SIZE	(PCI_VPD_ADDR_MASK + 1)
#define PCI_VPD_SZ_INVALID	UINT_MAX

/**
 * pci_vpd_size - determine actual size of Vital Product Data
 * @dev:	pci device struct
 */
static size_t pci_vpd_size(struct pci_dev *dev)
{
	size_t off = 0, size;
	unsigned char tag, header[1+2];	/* 1 byte tag, 2 bytes length */

	while (pci_read_vpd_any(dev, off, 1, header) == 1) {
		size = 0;

		if (off == 0 && (header[0] == 0x00 || header[0] == 0xff))
			goto error;

		if (header[0] & PCI_VPD_LRDT) {
			/* Large Resource Data Type Tag */
			if (pci_read_vpd_any(dev, off + 1, 2, &header[1]) != 2) {
				pci_warn(dev, "failed VPD read at offset %zu\n",
					 off + 1);
				return off ?: PCI_VPD_SZ_INVALID;
			}
			size = pci_vpd_lrdt_size(header);
			if (off + size > PCI_VPD_MAX_SIZE)
				goto error;

			off += PCI_VPD_LRDT_TAG_SIZE + size;
		} else {
			/* Short Resource Data Type Tag */
			tag = pci_vpd_srdt_tag(header);
			size = pci_vpd_srdt_size(header);
			if (off + size > PCI_VPD_MAX_SIZE)
				goto error;

			off += PCI_VPD_SRDT_TAG_SIZE + size;
			if (tag == PCI_VPD_STIN_END)	/* End tag descriptor */
				return off;
		}
	}
	return off;

error:
	pci_info(dev, "invalid VPD tag %#04x (size %zu) at offset %zu%s\n",
		 header[0], size, off, off == 0 ?
		 "; assume missing optional EEPROM" : "");
	return off ?: PCI_VPD_SZ_INVALID;
}

static bool pci_vpd_available(struct pci_dev *dev, bool check_size)
{
	struct pci_vpd *vpd = &dev->vpd;

	if (!vpd->cap)
		return false;

	if (vpd->len == 0 && check_size) {
		vpd->len = pci_vpd_size(dev);
		if (vpd->len == PCI_VPD_SZ_INVALID) {
			vpd->cap = 0;
			return false;
		}
	}

	return true;
}

/*
 * Wait for last operation to complete.
 * This code has to spin since there is no other notification from the PCI
 * hardware. Since the VPD is often implemented by serial attachment to an
 * EEPROM, it may take many milliseconds to complete.
 * @set: if true wait for flag to be set, else wait for it to be cleared
 *
 * Returns 0 on success, negative values indicate error.
 */
static int pci_vpd_wait(struct pci_dev *dev, bool set)
{
	struct pci_vpd *vpd = &dev->vpd;
	unsigned long timeout = jiffies + msecs_to_jiffies(125);
	unsigned long max_sleep = 16;
	u16 status;
	int ret;

	do {
		ret = pci_user_read_config_word(dev, vpd->cap + PCI_VPD_ADDR,
						&status);
		if (ret < 0)
			return ret;

		if (!!(status & PCI_VPD_ADDR_F) == set)
			return 0;

		if (time_after(jiffies, timeout))
			break;

		usleep_range(10, max_sleep);
		if (max_sleep < 1024)
			max_sleep *= 2;
	} while (true);

	pci_warn(dev, "VPD access failed.  This is likely a firmware bug on this device.  Contact the card vendor for a firmware update\n");
	return -ETIMEDOUT;
}

static ssize_t pci_vpd_read(struct pci_dev *dev, loff_t pos, size_t count,
			    void *arg, bool check_size)
{
	struct pci_vpd *vpd = &dev->vpd;
	unsigned int max_len;
	int ret = 0;
	loff_t end = pos + count;
	u8 *buf = arg;

	if (!pci_vpd_available(dev, check_size))
		return -ENODEV;

	if (pos < 0)
		return -EINVAL;

	max_len = check_size ? vpd->len : PCI_VPD_MAX_SIZE;

	if (pos >= max_len)
		return 0;

	if (end > max_len) {
		end = max_len;
		count = end - pos;
	}

	if (mutex_lock_killable(&vpd->lock))
		return -EINTR;

	while (pos < end) {
		u32 val;
		unsigned int i, skip;

		if (fatal_signal_pending(current)) {
			ret = -EINTR;
			break;
		}

		ret = pci_user_write_config_word(dev, vpd->cap + PCI_VPD_ADDR,
						 pos & ~3);
		if (ret < 0)
			break;
		ret = pci_vpd_wait(dev, true);
		if (ret < 0)
			break;

		ret = pci_user_read_config_dword(dev, vpd->cap + PCI_VPD_DATA, &val);
		if (ret < 0)
			break;

		skip = pos & 3;
		for (i = 0;  i < sizeof(u32); i++) {
			if (i >= skip) {
				*buf++ = val;
				if (++pos == end)
					break;
			}
			val >>= 8;
		}
	}

	mutex_unlock(&vpd->lock);
	return ret ? ret : count;
}

static ssize_t pci_vpd_write(struct pci_dev *dev, loff_t pos, size_t count,
			     const void *arg, bool check_size)
{
	struct pci_vpd *vpd = &dev->vpd;
	unsigned int max_len;
	const u8 *buf = arg;
	loff_t end = pos + count;
	int ret = 0;

	if (!pci_vpd_available(dev, check_size))
		return -ENODEV;

	if (pos < 0 || (pos & 3) || (count & 3))
		return -EINVAL;

	max_len = check_size ? vpd->len : PCI_VPD_MAX_SIZE;

	if (end > max_len)
		return -EINVAL;

	if (mutex_lock_killable(&vpd->lock))
		return -EINTR;

	while (pos < end) {
		ret = pci_user_write_config_dword(dev, vpd->cap + PCI_VPD_DATA,
						  get_unaligned_le32(buf));
		if (ret < 0)
			break;
		ret = pci_user_write_config_word(dev, vpd->cap + PCI_VPD_ADDR,
						 pos | PCI_VPD_ADDR_F);
		if (ret < 0)
			break;

		ret = pci_vpd_wait(dev, false);
		if (ret < 0)
			break;

		buf += sizeof(u32);
		pos += sizeof(u32);
	}

	mutex_unlock(&vpd->lock);
	return ret ? ret : count;
}

void pci_vpd_init(struct pci_dev *dev)
{
	if (dev->vpd.len == PCI_VPD_SZ_INVALID)
		return;

	dev->vpd.cap = pci_find_capability(dev, PCI_CAP_ID_VPD);
	mutex_init(&dev->vpd.lock);
}

static ssize_t vpd_read(struct file *filp, struct kobject *kobj,
			const struct bin_attribute *bin_attr, char *buf,
			loff_t off, size_t count)
{
	struct pci_dev *dev = to_pci_dev(kobj_to_dev(kobj));
	struct pci_dev *vpd_dev = dev;
	ssize_t ret;

	if (dev->dev_flags & PCI_DEV_FLAGS_VPD_REF_F0) {
		vpd_dev = pci_get_func0_dev(dev);
		if (!vpd_dev)
			return -ENODEV;
	}

	ret = pci_read_vpd(vpd_dev, off, count, buf);

	if (dev->dev_flags & PCI_DEV_FLAGS_VPD_REF_F0)
		pci_dev_put(vpd_dev);

	return ret;
}

static ssize_t vpd_write(struct file *filp, struct kobject *kobj,
			 const struct bin_attribute *bin_attr, char *buf,
			 loff_t off, size_t count)
{
	struct pci_dev *dev = to_pci_dev(kobj_to_dev(kobj));
	struct pci_dev *vpd_dev = dev;
	ssize_t ret;

	if (dev->dev_flags & PCI_DEV_FLAGS_VPD_REF_F0) {
		vpd_dev = pci_get_func0_dev(dev);
		if (!vpd_dev)
			return -ENODEV;
	}

	ret = pci_write_vpd(vpd_dev, off, count, buf);

	if (dev->dev_flags & PCI_DEV_FLAGS_VPD_REF_F0)
		pci_dev_put(vpd_dev);

	return ret;
}
static const BIN_ATTR(vpd, 0600, vpd_read, vpd_write, 0);

static const struct bin_attribute *const vpd_attrs[] = {
	&bin_attr_vpd,
	NULL,
};

static umode_t vpd_attr_is_visible(struct kobject *kobj,
				   const struct bin_attribute *a, int n)
{
	struct pci_dev *pdev = to_pci_dev(kobj_to_dev(kobj));

	if (!pdev->vpd.cap)
		return 0;

	return a->attr.mode;
}

const struct attribute_group pci_dev_vpd_attr_group = {
	.bin_attrs = vpd_attrs,
	.is_bin_visible = vpd_attr_is_visible,
};

void *pci_vpd_alloc(struct pci_dev *dev, unsigned int *size)
{
	unsigned int len;
	void *buf;
	int cnt;

	if (!pci_vpd_available(dev, true))
		return ERR_PTR(-ENODEV);

	len = dev->vpd.len;
	buf = kmalloc(len, GFP_KERNEL);
	if (!buf)
		return ERR_PTR(-ENOMEM);

	cnt = pci_read_vpd(dev, 0, len, buf);
	if (cnt != len) {
		kfree(buf);
		return ERR_PTR(-EIO);
	}

	if (size)
		*size = len;

	return buf;
}
EXPORT_SYMBOL_GPL(pci_vpd_alloc);

static int pci_vpd_find_tag(const u8 *buf, unsigned int len, u8 rdt, unsigned int *size)
{
	int i = 0;

	/* look for LRDT tags only, end tag is the only SRDT tag */
	while (i + PCI_VPD_LRDT_TAG_SIZE <= len && buf[i] & PCI_VPD_LRDT) {
		unsigned int lrdt_len = pci_vpd_lrdt_size(buf + i);
		u8 tag = buf[i];

		i += PCI_VPD_LRDT_TAG_SIZE;
		if (tag == rdt) {
			if (i + lrdt_len > len)
				lrdt_len = len - i;
			if (size)
				*size = lrdt_len;
			return i;
		}

		i += lrdt_len;
	}

	return -ENOENT;
}

int pci_vpd_find_id_string(const u8 *buf, unsigned int len, unsigned int *size)
{
	return pci_vpd_find_tag(buf, len, PCI_VPD_LRDT_ID_STRING, size);
}
EXPORT_SYMBOL_GPL(pci_vpd_find_id_string);

static int pci_vpd_find_info_keyword(const u8 *buf, unsigned int off,
			      unsigned int len, const char *kw)
{
	int i;

	for (i = off; i + PCI_VPD_INFO_FLD_HDR_SIZE <= off + len;) {
		if (buf[i + 0] == kw[0] &&
		    buf[i + 1] == kw[1])
			return i;

		i += PCI_VPD_INFO_FLD_HDR_SIZE +
		     pci_vpd_info_field_size(&buf[i]);
	}

	return -ENOENT;
}

static ssize_t __pci_read_vpd(struct pci_dev *dev, loff_t pos, size_t count, void *buf,
			      bool check_size)
{
	ssize_t ret;

	if (dev->dev_flags & PCI_DEV_FLAGS_VPD_REF_F0) {
		dev = pci_get_func0_dev(dev);
		if (!dev)
			return -ENODEV;

		ret = pci_vpd_read(dev, pos, count, buf, check_size);
		pci_dev_put(dev);
		return ret;
	}

	return pci_vpd_read(dev, pos, count, buf, check_size);
}

/**
 * pci_read_vpd - Read one entry from Vital Product Data
 * @dev:	PCI device struct
 * @pos:	offset in VPD space
 * @count:	number of bytes to read
 * @buf:	pointer to where to store result
 */
ssize_t pci_read_vpd(struct pci_dev *dev, loff_t pos, size_t count, void *buf)
{
	return __pci_read_vpd(dev, pos, count, buf, true);
}
EXPORT_SYMBOL(pci_read_vpd);

/* Same, but allow to access any address */
ssize_t pci_read_vpd_any(struct pci_dev *dev, loff_t pos, size_t count, void *buf)
{
	return __pci_read_vpd(dev, pos, count, buf, false);
}
EXPORT_SYMBOL(pci_read_vpd_any);

static ssize_t __pci_write_vpd(struct pci_dev *dev, loff_t pos, size_t count,
			       const void *buf, bool check_size)
{
	ssize_t ret;

	if (dev->dev_flags & PCI_DEV_FLAGS_VPD_REF_F0) {
		dev = pci_get_func0_dev(dev);
		if (!dev)
			return -ENODEV;

		ret = pci_vpd_write(dev, pos, count, buf, check_size);
		pci_dev_put(dev);
		return ret;
	}

	return pci_vpd_write(dev, pos, count, buf, check_size);
}

/**
 * pci_write_vpd - Write entry to Vital Product Data
 * @dev:	PCI device struct
 * @pos:	offset in VPD space
 * @count:	number of bytes to write
 * @buf:	buffer containing write data
 */
ssize_t pci_write_vpd(struct pci_dev *dev, loff_t pos, size_t count, const void *buf)
{
	return __pci_write_vpd(dev, pos, count, buf, true);
}
EXPORT_SYMBOL(pci_write_vpd);

/* Same, but allow to access any address */
ssize_t pci_write_vpd_any(struct pci_dev *dev, loff_t pos, size_t count, const void *buf)
{
	return __pci_write_vpd(dev, pos, count, buf, false);
}
EXPORT_SYMBOL(pci_write_vpd_any);

int pci_vpd_find_ro_info_keyword(const void *buf, unsigned int len,
				 const char *kw, unsigned int *size)
{
	int ro_start, infokw_start;
	unsigned int ro_len, infokw_size;

	ro_start = pci_vpd_find_tag(buf, len, PCI_VPD_LRDT_RO_DATA, &ro_len);
	if (ro_start < 0)
		return ro_start;

	infokw_start = pci_vpd_find_info_keyword(buf, ro_start, ro_len, kw);
	if (infokw_start < 0)
		return infokw_start;

	infokw_size = pci_vpd_info_field_size(buf + infokw_start);
	infokw_start += PCI_VPD_INFO_FLD_HDR_SIZE;

	if (infokw_start + infokw_size > len)
		return -EINVAL;

	if (size)
		*size = infokw_size;

	return infokw_start;
}
EXPORT_SYMBOL_GPL(pci_vpd_find_ro_info_keyword);

int pci_vpd_check_csum(const void *buf, unsigned int len)
{
	const u8 *vpd = buf;
	unsigned int size;
	u8 csum = 0;
	int rv_start;

	rv_start = pci_vpd_find_ro_info_keyword(buf, len, PCI_VPD_RO_KEYWORD_CHKSUM, &size);
	if (rv_start == -ENOENT) /* no checksum in VPD */
		return 1;
	else if (rv_start < 0)
		return rv_start;

	if (!size)
		return -EINVAL;

	while (rv_start >= 0)
		csum += vpd[rv_start--];

	return csum ? -EILSEQ : 0;
}
EXPORT_SYMBOL_GPL(pci_vpd_check_csum);
