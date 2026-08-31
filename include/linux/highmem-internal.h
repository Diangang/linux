/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_HIGHMEM_INTERNAL_H
#define _LINUX_HIGHMEM_INTERNAL_H

/*
 * Outside of CONFIG_HIGHMEM to support X86 32bit iomap_atomic() cruft.
 */
static inline void kmap_local_fork(struct task_struct *tsk) { }
static inline void kmap_assert_nomap(void) { }


static inline struct page *kmap_to_page(void *addr)
{
	return virt_to_page(addr);
}

static inline void *kmap(struct page *page)
{
	might_sleep();
	return page_address(page);
}

static inline void kunmap_high(const struct page *page) { }
static inline void kmap_flush_unused(void) { }

static inline void kunmap(const struct page *page)
{
#ifdef ARCH_HAS_FLUSH_ON_KUNMAP
	kunmap_flush_on_unmap(page_address(page));
#endif
}

static inline void *kmap_local_page(const struct page *page)
{
	return page_address(page);
}

static inline void *kmap_local_page_try_from_panic(const struct page *page)
{
	return page_address(page);
}

static inline void *kmap_local_folio(const struct folio *folio, size_t offset)
{
	return folio_address(folio) + offset;
}

static inline void *kmap_local_page_prot(const struct page *page, pgprot_t prot)
{
	return kmap_local_page(page);
}

static inline void *kmap_local_pfn(unsigned long pfn)
{
	return kmap_local_page(pfn_to_page(pfn));
}

static inline void __kunmap_local(const void *addr)
{
#ifdef ARCH_HAS_FLUSH_ON_KUNMAP
	kunmap_flush_on_unmap(PTR_ALIGN_DOWN(addr, PAGE_SIZE));
#endif
}

static inline void *kmap_atomic(const struct page *page)
{
	preempt_disable();
	pagefault_disable();
	return page_address(page);
}

static inline void *kmap_atomic_prot(const struct page *page, pgprot_t prot)
{
	return kmap_atomic(page);
}

static inline void *kmap_atomic_pfn(unsigned long pfn)
{
	return kmap_atomic(pfn_to_page(pfn));
}

static inline void __kunmap_atomic(const void *addr)
{
#ifdef ARCH_HAS_FLUSH_ON_KUNMAP
	kunmap_flush_on_unmap(PTR_ALIGN_DOWN(addr, PAGE_SIZE));
#endif
	pagefault_enable();
	preempt_enable();
}

static inline unsigned long nr_free_highpages(void) { return 0; }
static inline unsigned long totalhigh_pages(void) { return 0; }

static inline bool is_kmap_addr(const void *x)
{
	return false;
}


/**
 * kunmap_atomic - Unmap the virtual address mapped by kmap_atomic() - deprecated!
 * @__addr:       Virtual address to be unmapped
 *
 * Unmaps an address previously mapped by kmap_atomic() and re-enables
 * pagefaults. Depending on PREEMP_RT configuration, re-enables also
 * migration and preemption. Users should not count on these side effects.
 *
 * Mappings should be unmapped in the reverse order that they were mapped.
 * See kmap_local_page() for details on nesting.
 *
 * @__addr can be any address within the mapped page, so there is no need
 * to subtract any offset that has been added. In contrast to kunmap(),
 * this function takes the address returned from kmap_atomic(), not the
 * page passed to it. The compiler will warn you if you pass the page.
 */
#define kunmap_atomic(__addr)					\
do {								\
	BUILD_BUG_ON(__same_type((__addr), struct page *));	\
	__kunmap_atomic(__addr);				\
} while (0)

/**
 * kunmap_local - Unmap a page mapped via kmap_local_page().
 * @__addr: An address within the page mapped
 *
 * @__addr can be any address within the mapped page.  Commonly it is the
 * address return from kmap_local_page(), but it can also include offsets.
 *
 * Unmapping should be done in the reverse order of the mapping.  See
 * kmap_local_page() for details.
 */
#define kunmap_local(__addr)					\
do {								\
	BUILD_BUG_ON(__same_type((__addr), struct page *));	\
	__kunmap_local(__addr);					\
} while (0)

#endif
