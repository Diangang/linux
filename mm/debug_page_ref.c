// SPDX-License-Identifier: GPL-2.0
#include <linux/mm_types.h>
#include <linux/tracepoint.h>
void __page_ref_set(struct page *page, int v)
{
}
EXPORT_SYMBOL(__page_ref_set);
void __page_ref_mod(struct page *page, int v)
{
}
EXPORT_SYMBOL(__page_ref_mod);
void __page_ref_mod_and_test(struct page *page, int v, int ret)
{
}
EXPORT_SYMBOL(__page_ref_mod_and_test);
void __page_ref_mod_and_return(struct page *page, int v, int ret)
{
}
EXPORT_SYMBOL(__page_ref_mod_and_return);
void __page_ref_mod_unless(struct page *page, int v, int u)
{
}
EXPORT_SYMBOL(__page_ref_mod_unless);
void __page_ref_freeze(struct page *page, int v, int ret)
{
}
EXPORT_SYMBOL(__page_ref_freeze);
void __page_ref_unfreeze(struct page *page, int v)
{
}
EXPORT_SYMBOL(__page_ref_unfreeze);
