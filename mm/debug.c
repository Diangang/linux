// SPDX-License-Identifier: GPL-2.0
/*
 * mm/debug.c
 *
 * mm/ specific debug routines.
 *
 */

#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/trace_events.h>
#include <linux/migrate.h>
#include <linux/mmflag_names.h>
#include <linux/page_owner.h>
#include <linux/ctype.h>

#include "internal.h"

/* Define EM() and EMe() so MIGRATE_REASON can populate migrate_reason_names[]. */
#undef EM
#undef EMe
#define EM(a, b)	b,
#define EMe(a, b)	b

const char *migrate_reason_names[MR_TYPES] = {
	MIGRATE_REASON
};

const struct trace_print_flags pageflag_names[] = {
	__def_pageflag_names,
	{0, NULL}
};

const struct trace_print_flags gfpflag_names[] = {
	__def_gfpflag_names,
	{0, NULL}
};

const struct trace_print_flags vmaflag_names[] = {
	__def_vmaflag_names,
	{0, NULL}
};

#define DEF_PAGETYPE_NAME(_name) [PGTY_##_name - 0xf0] =  __stringify(_name)

static const char *page_type_names[] = {
	DEF_PAGETYPE_NAME(slab),
	DEF_PAGETYPE_NAME(hugetlb),
	DEF_PAGETYPE_NAME(offline),
	DEF_PAGETYPE_NAME(guard),
	DEF_PAGETYPE_NAME(table),
	DEF_PAGETYPE_NAME(buddy),
	DEF_PAGETYPE_NAME(unaccepted),
};

static const char *page_type_name(unsigned int page_type)
{
	unsigned i = (page_type >> 24) - 0xf0;

	if (i >= ARRAY_SIZE(page_type_names))
		return "unknown";
	return page_type_names[i];
}

static void __dump_folio(const struct folio *folio, const struct page *page,
		unsigned long pfn, unsigned long idx)
{
	struct address_space *mapping = folio_mapping(folio);
	int mapcount = atomic_read(&page->_mapcount) + 1;
	char *type = "";

	if (page_mapcount_is_type(mapcount))
		mapcount = 0;

	pr_warn("page: refcount:%d mapcount:%d mapping:%p index:%#lx pfn:%#lx\n",
			folio_ref_count(folio), mapcount, mapping,
			folio->index + idx, pfn);
	if (folio_test_large(folio)) {
		int pincount = 0;

		if (folio_has_pincount(folio))
			pincount = atomic_read(&folio->_pincount);

		pr_warn("head: order:%u mapcount:%d entire_mapcount:%d nr_pages_mapped:%d pincount:%d\n",
				folio_order(folio),
				folio_mapcount(folio),
				folio_entire_mapcount(folio),
				folio_nr_pages_mapped(folio),
				pincount);
	}

	if (folio_test_ksm(folio))
		type = "ksm ";
	else if (folio_test_anon(folio))
		type = "anon ";
	else if (mapping)
		dump_mapping(mapping);
	BUILD_BUG_ON(ARRAY_SIZE(pageflag_names) != __NR_PAGEFLAGS + 1);

	/*
	 * Accessing the pageblock without the zone lock. It could change to
	 * "isolate" again in the meantime, but since we are just dumping the
	 * state for debugging, it should be fine to accept a bit of
	 * inaccuracy here due to racing.
	 */
	pr_warn("%sflags: %pGp%s\n", type, &folio->flags,
		is_migrate_cma_folio(folio, pfn) ? " CMA" : "");
	if (page_has_type(&folio->page))
		pr_warn("page_type: %x(%s)\n", folio->page.page_type >> 24,
				page_type_name(folio->page.page_type));

	print_hex_dump(KERN_WARNING, "raw: ", DUMP_PREFIX_NONE, 32,
			sizeof(unsigned long), page,
			sizeof(struct page), false);
	if (folio_test_large(folio))
		print_hex_dump(KERN_WARNING, "head: ", DUMP_PREFIX_NONE, 32,
			sizeof(unsigned long), folio,
			2 * sizeof(struct page), false);
}

static void __dump_page(const struct page *page)
{
	struct page_snapshot ps;

	snapshot_page(&ps, page);
	if (!snapshot_page_is_faithful(&ps))
		pr_warn("page does not match folio\n");

	__dump_folio(&ps.folio_snapshot, &ps.page_snapshot, ps.pfn, ps.idx);
}

void dump_page(const struct page *page, const char *reason)
{
	__dump_page(page);
	if (reason)
		pr_warn("page dumped because: %s\n", reason);
	dump_page_owner(page);
}
EXPORT_SYMBOL(dump_page);
