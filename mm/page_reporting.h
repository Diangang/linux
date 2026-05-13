/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _MM_PAGE_REPORTING_H
#define _MM_PAGE_REPORTING_H

#include <linux/mmzone.h>
#include <linux/pageblock-flags.h>
#include <linux/page-isolation.h>
#include <linux/jump_label.h>
#include <linux/slab.h>
#include <linux/pgtable.h>
#include <linux/scatterlist.h>

#define page_reported(_page)	false

static inline void page_reporting_notify_free(unsigned int order)
{
}
#endif /*_MM_PAGE_REPORTING_H */
