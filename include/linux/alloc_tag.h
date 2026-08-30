/* SPDX-License-Identifier: GPL-2.0 */
/*
 * allocation tagging
 */
#ifndef _LINUX_ALLOC_TAG_H
#define _LINUX_ALLOC_TAG_H

#include <linux/bug.h>
#include <linux/codetag.h>
#include <linux/container_of.h>
#include <linux/preempt.h>
#include <asm/percpu.h>
#include <linux/cpumask.h>
#include <linux/smp.h>
#include <linux/static_key.h>
#include <linux/irqflags.h>

struct alloc_tag_counters {
	u64 bytes;
	u64 calls;
};

/*
 * An instance of this structure is created in a special ELF section at every
 * allocation callsite. At runtime, the special section is treated as
 * an array of these. Embedded codetag utilizes codetag framework.
 */
struct alloc_tag {
	struct codetag			ct;
	struct alloc_tag_counters __percpu	*counters;
} __aligned(8);

struct alloc_tag_kernel_section {
	struct alloc_tag *first_tag;
	unsigned long count;
};

struct alloc_tag_module_section {
	union {
		unsigned long start_addr;
		struct alloc_tag *first_tag;
	};
	unsigned long end_addr;
	/* used size */
	unsigned long size;
};


static inline bool is_codetag_empty(union codetag_ref *ref) { return false; }

static inline void set_codetag_empty(union codetag_ref *ref)
{
	if (ref)
		ref->ct = NULL;
}



#define DEFINE_ALLOC_TAG(_alloc_tag)
static inline bool mem_alloc_profiling_enabled(void) { return false; }
static inline void alloc_tag_add(union codetag_ref *ref, struct alloc_tag *tag,
				 size_t bytes) {}
static inline void alloc_tag_sub(union codetag_ref *ref, size_t bytes) {}
static inline void alloc_tag_set_inaccurate(struct alloc_tag *tag) {}
static inline bool alloc_tag_is_inaccurate(struct alloc_tag *tag) { return false; }
#define alloc_tag_record(p)	do {} while (0)


#define alloc_hooks_tag(_tag, _do_alloc)				\
({									\
	typeof(_do_alloc) _res;						\
	if (mem_alloc_profiling_enabled()) {				\
		struct alloc_tag * __maybe_unused _old;			\
		_old = alloc_tag_save(_tag);				\
		_res = _do_alloc;					\
		alloc_tag_restore(_tag, _old);				\
	} else								\
		_res = _do_alloc;					\
	_res;								\
})

#define alloc_hooks(_do_alloc)						\
({									\
	DEFINE_ALLOC_TAG(_alloc_tag);					\
	alloc_hooks_tag(&_alloc_tag, _do_alloc);			\
})

#endif /* _LINUX_ALLOC_TAG_H */
