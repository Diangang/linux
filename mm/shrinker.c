// SPDX-License-Identifier: GPL-2.0
#include <linux/rwsem.h>
#include <linux/shrinker.h>
#include <linux/rculist.h>

#include "internal.h"

LIST_HEAD(shrinker_list);
DEFINE_MUTEX(shrinker_mutex);

static long xchg_nr_deferred(struct shrinker *shrinker,
			     struct shrink_control *sc)
{
	int nid = sc->nid;

	if (!(shrinker->flags & SHRINKER_NUMA_AWARE))
		nid = 0;

	return atomic_long_xchg(&shrinker->nr_deferred[nid], 0);
}


static long add_nr_deferred(long nr, struct shrinker *shrinker,
			    struct shrink_control *sc)
{
	int nid = sc->nid;

	if (!(shrinker->flags & SHRINKER_NUMA_AWARE))
		nid = 0;

	return atomic_long_add_return(nr, &shrinker->nr_deferred[nid]);
}

#define SHRINK_BATCH 128

static unsigned long do_shrink_slab(struct shrink_control *shrinkctl,
				    struct shrinker *shrinker, int priority)
{
	unsigned long freed = 0;
	unsigned long long delta;
	long total_scan;
	long freeable;
	long nr;
	long batch_size = shrinker->batch ? shrinker->batch
					  : SHRINK_BATCH;
	long scanned = 0, next_deferred;

	freeable = shrinker->count_objects(shrinker, shrinkctl);
	if (freeable == 0 || freeable == SHRINK_EMPTY)
		return freeable;

	/*
	 * copy the current shrinker scan count into a local variable
	 * and zero it so that other concurrent shrinker invocations
	 * don't also do this scanning work.
	 */
	nr = xchg_nr_deferred(shrinker, shrinkctl);

	if (shrinker->seeks) {
		delta = freeable >> priority;
		delta *= 4;
		do_div(delta, shrinker->seeks);
	} else {
		/*
		 * These objects don't require any IO to create. Trim
		 * them aggressively under memory pressure to keep
		 * them from causing refetches in the IO caches.
		 */
		delta = freeable / 2;
	}

	total_scan = nr >> priority;
	total_scan += delta;
	total_scan = min(total_scan, (2 * freeable));


	/*
	 * Normally, we should not scan less than batch_size objects in one
	 * pass to avoid too frequent shrinker calls, but if the slab has less
	 * than batch_size objects in total and we are really tight on memory,
	 * we will try to reclaim all available objects, otherwise we can end
	 * up failing allocations although there are plenty of reclaimable
	 * objects spread over several slabs with usage less than the
	 * batch_size.
	 *
	 * We detect the "tight on memory" situations by looking at the total
	 * number of objects we want to scan (total_scan). If it is greater
	 * than the total number of objects on slab (freeable), we must be
	 * scanning at high prio and therefore should try to reclaim as much as
	 * possible.
	 */
	while (total_scan >= batch_size ||
	       total_scan >= freeable) {
		unsigned long ret;
		unsigned long nr_to_scan = min(batch_size, total_scan);

		shrinkctl->nr_to_scan = nr_to_scan;
		shrinkctl->nr_scanned = nr_to_scan;
		ret = shrinker->scan_objects(shrinker, shrinkctl);
		if (ret == SHRINK_STOP)
			break;
		freed += ret;

		count_vm_events(SLABS_SCANNED, shrinkctl->nr_scanned);
		total_scan -= shrinkctl->nr_scanned;
		scanned += shrinkctl->nr_scanned;

		cond_resched();
	}

	/*
	 * The deferred work is increased by any new work (delta) that wasn't
	 * done, decreased by old deferred work that was done now.
	 *
	 * And it is capped to two times of the freeable items.
	 */
	next_deferred = max_t(long, (nr + delta - scanned), 0);
	next_deferred = min(next_deferred, (2 * freeable));

	/*
	 * move the unused scan count back into the shrinker in a
	 * manner that handles concurrent updates.
	 */
	add_nr_deferred(next_deferred, shrinker, shrinkctl);

	return freed;
}

/**
 * shrink_slab - shrink slab caches
 * @gfp_mask: allocation context
 * @nid: node whose slab caches to target
 * @priority: the reclaim priority
 *
 * Call the shrink functions to age shrinkable caches.
 *
 * @nid is passed along to shrinkers with SHRINKER_NUMA_AWARE set,
 * unaware shrinkers will receive a node id of 0 instead.
 *
 * @priority is sc->priority, we take the number of objects and >> by priority
 * in order to get the scan target.
 *
 * Returns the number of reclaimed slab objects.
 */
unsigned long shrink_slab(gfp_t gfp_mask, int nid, int priority)
{
	unsigned long ret, freed = 0;
	struct shrinker *shrinker;

	/*
	 * lockless algorithm of global shrink.
	 *
	 * In the unregistration setp, the shrinker will be freed asynchronously
	 * via RCU after its refcount reaches 0. So both rcu_read_lock() and
	 * shrinker_try_get() can be used to ensure the existence of the shrinker.
	 *
	 * So in the global shrink:
	 *  step 1: use rcu_read_lock() to guarantee existence of the shrinker
	 *          and the validity of the shrinker_list walk.
	 *  step 2: use shrinker_try_get() to try get the refcount, if successful,
	 *          then the existence of the shrinker can also be guaranteed,
	 *          so we can release the RCU lock to do do_shrink_slab() that
	 *          may sleep.
	 *  step 3: *MUST* to reacquire the RCU lock before calling shrinker_put(),
	 *          which ensures that neither this shrinker nor the next shrinker
	 *          will be freed in the next traversal operation.
	 *  step 4: do shrinker_put() paired with step 2 to put the refcount,
	 *          if the refcount reaches 0, then wake up the waiter in
	 *          shrinker_free() by calling complete().
	 */
	rcu_read_lock();
	list_for_each_entry_rcu(shrinker, &shrinker_list, list) {
		struct shrink_control sc = {
			.gfp_mask = gfp_mask,
			.nid = nid,
		};

		if (!shrinker_try_get(shrinker))
			continue;

		rcu_read_unlock();

		ret = do_shrink_slab(&sc, shrinker, priority);
		if (ret == SHRINK_EMPTY)
			ret = 0;
		freed += ret;

		rcu_read_lock();
		shrinker_put(shrinker);
	}

	rcu_read_unlock();
	cond_resched();
	return freed;
}

struct shrinker *shrinker_alloc(unsigned int flags, const char *fmt, ...)
{
	struct shrinker *shrinker;
	unsigned int size;
	va_list ap;
	int err;

	shrinker = kzalloc_obj(struct shrinker);
	if (!shrinker)
		return NULL;

	va_start(ap, fmt);
	err = shrinker_debugfs_name_alloc(shrinker, fmt, ap);
	va_end(ap);
	if (err)
		goto err_name;

	shrinker->flags = flags | SHRINKER_ALLOCATED;
	shrinker->seeks = DEFAULT_SEEKS;

	size = sizeof(*shrinker->nr_deferred);
	if (flags & SHRINKER_NUMA_AWARE)
		size *= nr_node_ids;

	shrinker->nr_deferred = kzalloc(size, GFP_KERNEL);
	if (!shrinker->nr_deferred)
		goto err_flags;

	return shrinker;

err_flags:
	shrinker_debugfs_name_free(shrinker);
err_name:
	kfree(shrinker);
	return NULL;
}
EXPORT_SYMBOL_GPL(shrinker_alloc);

void shrinker_register(struct shrinker *shrinker)
{
	if (unlikely(!(shrinker->flags & SHRINKER_ALLOCATED))) {
		pr_warn("Must use shrinker_alloc() to dynamically allocate the shrinker");
		return;
	}

	mutex_lock(&shrinker_mutex);
	list_add_tail_rcu(&shrinker->list, &shrinker_list);
	shrinker->flags |= SHRINKER_REGISTERED;
	shrinker_debugfs_add(shrinker);
	mutex_unlock(&shrinker_mutex);

	init_completion(&shrinker->done);
	/*
	 * Now the shrinker is fully set up, take the first reference to it to
	 * indicate that lookup operations are now allowed to use it via
	 * shrinker_try_get().
	 */
	refcount_set(&shrinker->refcount, 1);
}
EXPORT_SYMBOL_GPL(shrinker_register);

static void shrinker_free_rcu_cb(struct rcu_head *head)
{
	struct shrinker *shrinker = container_of(head, struct shrinker, rcu);

	kfree(shrinker->nr_deferred);
	kfree(shrinker);
}

void shrinker_free(struct shrinker *shrinker)
{
	struct dentry *debugfs_entry = NULL;
	int debugfs_id;

	if (!shrinker)
		return;

	if (shrinker->flags & SHRINKER_REGISTERED) {
		/* drop the initial refcount */
		shrinker_put(shrinker);
		/*
		 * Wait for all lookups of the shrinker to complete, after that,
		 * no shrinker is running or will run again, then we can safely
		 * free it asynchronously via RCU and safely free the structure
		 * where the shrinker is located, such as super_block etc.
		 */
		wait_for_completion(&shrinker->done);
	}

	mutex_lock(&shrinker_mutex);
	if (shrinker->flags & SHRINKER_REGISTERED) {
		/*
		 * Now we can safely remove it from the shrinker_list and then
		 * free it.
		 */
		list_del_rcu(&shrinker->list);
		debugfs_entry = shrinker_debugfs_detach(shrinker, &debugfs_id);
		shrinker->flags &= ~SHRINKER_REGISTERED;
	}

	shrinker_debugfs_name_free(shrinker);

	mutex_unlock(&shrinker_mutex);

	if (debugfs_entry)
		shrinker_debugfs_remove(debugfs_entry, debugfs_id);

	call_rcu(&shrinker->rcu, shrinker_free_rcu_cb);
}
EXPORT_SYMBOL_GPL(shrinker_free);
