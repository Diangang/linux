// SPDX-License-Identifier: GPL-2.0
#include <linux/init.h>
#include <linux/static_call.h>
#include <linux/bug.h>
#include <linux/smp.h>
#include <linux/sort.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/cpu.h>
#include <linux/processor.h>
#include <asm/sections.h>

extern struct static_call_site __start_static_call_sites[],
			       __stop_static_call_sites[];
extern struct static_call_tramp_key __start_static_call_tramp_key[],
				    __stop_static_call_tramp_key[];

int static_call_initialized;

/*
 * Must be called before early_initcall() to be effective.
 */
void static_call_force_reinit(void)
{
	if (WARN_ON_ONCE(!static_call_initialized))
		return;

	static_call_initialized++;
}

/* mutex to protect key modules/sites */
static DEFINE_MUTEX(static_call_mutex);

static void static_call_lock(void)
{
	mutex_lock(&static_call_mutex);
}

static void static_call_unlock(void)
{
	mutex_unlock(&static_call_mutex);
}

static inline void *static_call_addr(struct static_call_site *site)
{
	return (void *)((long)site->addr + (long)&site->addr);
}

static inline unsigned long __static_call_key(const struct static_call_site *site)
{
	return (long)site->key + (long)&site->key;
}

static inline struct static_call_key *static_call_key(const struct static_call_site *site)
{
	return (void *)(__static_call_key(site) & ~STATIC_CALL_SITE_FLAGS);
}

/* These assume the key is word-aligned. */
static inline bool static_call_is_init(struct static_call_site *site)
{
	return __static_call_key(site) & STATIC_CALL_SITE_INIT;
}

static inline bool static_call_is_tail(struct static_call_site *site)
{
	return __static_call_key(site) & STATIC_CALL_SITE_TAIL;
}

static inline void static_call_set_init(struct static_call_site *site)
{
	site->key = (__static_call_key(site) | STATIC_CALL_SITE_INIT) -
		    (long)&site->key;
}

static int static_call_site_cmp(const void *_a, const void *_b)
{
	const struct static_call_site *a = _a;
	const struct static_call_site *b = _b;
	const struct static_call_key *key_a = static_call_key(a);
	const struct static_call_key *key_b = static_call_key(b);

	if (key_a < key_b)
		return -1;

	if (key_a > key_b)
		return 1;

	return 0;
}

static void static_call_site_swap(void *_a, void *_b, int size)
{
	long delta = (unsigned long)_a - (unsigned long)_b;
	struct static_call_site *a = _a;
	struct static_call_site *b = _b;
	struct static_call_site tmp = *a;

	a->addr = b->addr  - delta;
	a->key  = b->key   - delta;

	b->addr = tmp.addr + delta;
	b->key  = tmp.key  + delta;
}

static inline void static_call_sort_entries(struct static_call_site *start,
					    struct static_call_site *stop)
{
	sort(start, stop - start, sizeof(struct static_call_site),
	     static_call_site_cmp, static_call_site_swap);
}

static inline bool static_call_key_has_mods(struct static_call_key *key)
{
	return !(key->type & 1);
}

static inline struct static_call_mod *static_call_key_next(struct static_call_key *key)
{
	if (!static_call_key_has_mods(key))
		return NULL;

	return key->mods;
}

static inline struct static_call_site *static_call_key_sites(struct static_call_key *key)
{
	if (static_call_key_has_mods(key))
		return NULL;

	return (struct static_call_site *)(key->type & ~1);
}

void __static_call_update(struct static_call_key *key, void *tramp, void *func)
{
	struct static_call_site *site, *stop;
	struct static_call_mod *site_mod, first;

	cpus_read_lock();
	static_call_lock();

	if (key->func == func)
		goto done;

	key->func = func;

	arch_static_call_transform(NULL, tramp, func, false);

	/*
	 * If uninitialized, we'll not update the callsites, but they still
	 * point to the trampoline and we just patched that.
	 */
	if (WARN_ON_ONCE(!static_call_initialized))
		goto done;

	first = (struct static_call_mod){
		.next = static_call_key_next(key),
		.mod = NULL,
		.sites = static_call_key_sites(key),
	};

	for (site_mod = &first; site_mod; site_mod = site_mod->next) {
		bool init = system_state < SYSTEM_RUNNING;

		if (!site_mod->sites) {
			continue;
		}

		stop = __stop_static_call_sites;

		for (site = site_mod->sites;
		     site < stop && static_call_key(site) == key; site++) {
			void *site_addr = static_call_addr(site);

			if (!init && static_call_is_init(site))
				continue;

			if (!kernel_text_address((unsigned long)site_addr)) {
				/*
				 * This skips patching built-in __exit, which
				 * is part of init_section_contains() but is
				 * not part of kernel_text_address().
				 *
				 * Skipping built-in __exit is fine since it
				 * will never be executed.
				 */
				WARN_ONCE(!static_call_is_init(site),
					  "can't patch static call site at %pS",
					  site_addr);
				continue;
			}

			arch_static_call_transform(site_addr, tramp, func,
						   static_call_is_tail(site));
		}
	}

done:
	static_call_unlock();
	cpus_read_unlock();
}
EXPORT_SYMBOL_GPL(__static_call_update);

static int __static_call_init(struct module *mod,
			      struct static_call_site *start,
			      struct static_call_site *stop)
{
	struct static_call_site *site;
	struct static_call_key *key, *prev_key = NULL;
	struct static_call_mod *site_mod;

	if (start == stop)
		return 0;

	static_call_sort_entries(start, stop);

	for (site = start; site < stop; site++) {
		void *site_addr = static_call_addr(site);

		if ((mod && within_module_init((unsigned long)site_addr, mod)) ||
		    (!mod && init_section_contains(site_addr, 1)))
			static_call_set_init(site);

		key = static_call_key(site);
		if (key != prev_key) {
			prev_key = key;

			/*
			 * For vmlinux (!mod) avoid the allocation by storing
			 * the sites pointer in the key itself. Also see
			 * __static_call_update()'s @first.
			 *
			 * This allows architectures (eg. x86) to call
			 * static_call_init() before memory allocation works.
			 */
			if (!mod) {
				key->sites = site;
				key->type |= 1;
				goto do_transform;
			}

			site_mod = kzalloc_obj(*site_mod);
			if (!site_mod)
				return -ENOMEM;

			/*
			 * When the key has a direct sites pointer, extract
			 * that into an explicit struct static_call_mod, so we
			 * can have a list of modules.
			 */
			if (static_call_key_sites(key)) {
				site_mod->mod = NULL;
				site_mod->next = NULL;
				site_mod->sites = static_call_key_sites(key);

				key->mods = site_mod;

				site_mod = kzalloc_obj(*site_mod);
				if (!site_mod)
					return -ENOMEM;
			}

			site_mod->mod = mod;
			site_mod->sites = site;
			site_mod->next = static_call_key_next(key);
			key->mods = site_mod;
		}

do_transform:
		arch_static_call_transform(site_addr, NULL, key->func,
				static_call_is_tail(site));
	}

	return 0;
}

static int addr_conflict(struct static_call_site *site, void *start, void *end)
{
	unsigned long addr = (unsigned long)static_call_addr(site);

	if (addr <= (unsigned long)end &&
	    addr + CALL_INSN_SIZE > (unsigned long)start)
		return 1;

	return 0;
}

static int __static_call_text_reserved(struct static_call_site *iter_start,
				       struct static_call_site *iter_stop,
				       void *start, void *end, bool init)
{
	struct static_call_site *iter = iter_start;

	while (iter < iter_stop) {
		if (init || !static_call_is_init(iter)) {
			if (addr_conflict(iter, start, end))
				return 1;
		}
		iter++;
	}

	return 0;
}

static inline int __static_call_mod_text_reserved(void *start, void *end)
{
	return 0;
}

int static_call_text_reserved(void *start, void *end)
{
	bool init = system_state < SYSTEM_RUNNING;
	int ret = __static_call_text_reserved(__start_static_call_sites,
			__stop_static_call_sites, start, end, init);

	if (ret)
		return ret;

	return __static_call_mod_text_reserved(start, end);
}

int __init static_call_init(void)
{
	int ret;

	/* See static_call_force_reinit(). */
	if (static_call_initialized == 1)
		return 0;

	cpus_read_lock();
	static_call_lock();
	ret = __static_call_init(NULL, __start_static_call_sites,
				 __stop_static_call_sites);
	static_call_unlock();
	cpus_read_unlock();

	if (ret) {
		pr_err("Failed to allocate memory for static_call!\n");
		BUG();
	}

	static_call_initialized = 1;
	return 0;
}
early_initcall(static_call_init);
