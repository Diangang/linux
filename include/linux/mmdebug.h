/* SPDX-License-Identifier: GPL-2.0 */
#ifndef LINUX_MM_DEBUG_H
#define LINUX_MM_DEBUG_H 1

#include <linux/bug.h>
#include <linux/stringify.h>

struct page;
struct vm_area_struct;
struct mm_struct;
struct vma_iterator;
struct vma_merge_struct;

void dump_page(const struct page *page, const char *reason);
void dump_vma(const struct vm_area_struct *vma);
void dump_mm(const struct mm_struct *mm);
void dump_vmg(const struct vma_merge_struct *vmg, const char *reason);
void vma_iter_dump_tree(const struct vma_iterator *vmi);

#define VM_BUG_ON(cond) BUILD_BUG_ON_INVALID(cond)
#define VM_BUG_ON_PAGE(cond, page) VM_BUG_ON(cond)
#define VM_BUG_ON_FOLIO(cond, folio) VM_BUG_ON(cond)
#define VM_BUG_ON_VMA(cond, vma) VM_BUG_ON(cond)
#define VM_BUG_ON_MM(cond, mm) VM_BUG_ON(cond)
#define VM_WARN_ON(cond) BUILD_BUG_ON_INVALID(cond)
#define VM_WARN_ON_ONCE(cond) BUILD_BUG_ON_INVALID(cond)
#define VM_WARN_ON_PAGE(cond, page)  BUILD_BUG_ON_INVALID(cond)
#define VM_WARN_ON_ONCE_PAGE(cond, page)  BUILD_BUG_ON_INVALID(cond)
#define VM_WARN_ON_FOLIO(cond, folio)  BUILD_BUG_ON_INVALID(cond)
#define VM_WARN_ON_ONCE_FOLIO(cond, folio)  BUILD_BUG_ON_INVALID(cond)
#define VM_WARN_ON_ONCE_MM(cond, mm)  BUILD_BUG_ON_INVALID(cond)
#define VM_WARN_ON_ONCE_VMA(cond, vma)  BUILD_BUG_ON_INVALID(cond)
#define VM_WARN_ON_VMG(cond, vmg)  BUILD_BUG_ON_INVALID(cond)
#define VM_WARN_ONCE(cond, format...) BUILD_BUG_ON_INVALID(cond)
#define VM_WARN(cond, format...) BUILD_BUG_ON_INVALID(cond)

#ifdef CONFIG_DEBUG_VM_IRQSOFF
#define VM_WARN_ON_IRQS_ENABLED() WARN_ON_ONCE(!irqs_disabled())
#else
#define VM_WARN_ON_IRQS_ENABLED() do { } while (0)
#endif

#define VIRTUAL_BUG_ON(cond) do { } while (0)

#ifdef CONFIG_DEBUG_VM_PGFLAGS
#define VM_BUG_ON_PGFLAGS(cond, page) VM_BUG_ON_PAGE(cond, page)
#else
#define VM_BUG_ON_PGFLAGS(cond, page) BUILD_BUG_ON_INVALID(cond)
#endif

#endif
