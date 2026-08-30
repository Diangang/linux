// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2013 Red Hat, Inc. and Parallels Inc. All rights reserved.
 * Authors: David Chinner and Glauber Costa
 *
 * Generic LRU infrastructure
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/list_lru.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/memcontrol.h>
#include "slab.h"
#include "internal.h"

static void list_lru_register(struct list_lru *lru)
{
}

static void list_lru_unregister(struct list_lru *lru)
{
}

static int lru_shrinker_id(struct list_lru *lru)
{
	return -1;
}

static inline bool list_lru_memcg_aware(struct list_lru *lru)
{
	return false;
}

static inline struct list_lru_one *
list_lru_from_memcg_idx(struct list_lru *lru, int nid, int idx)
{
	return &lru->node[nid].lru;
}

static inline struct list_lru_one *
lock_list_lru_of_memcg(struct list_lru *lru, int nid, struct mem_cgroup *memcg,
		       bool irq, bool skip_empty)
{
	struct list_lru_one *l = &lru->node[nid].lru;

	if (irq)
		spin_lock_irq(&l->lock);
	else
		spin_lock(&l->lock);

	return l;
}

static inline void unlock_list_lru(struct list_lru_one *l, bool irq_off)
{
	if (irq_off)
		spin_unlock_irq(&l->lock);
	else
		spin_unlock(&l->lock);
}

/* The caller must ensure the memcg lifetime. */
bool list_lru_add(struct list_lru *lru, struct list_head *item, int nid,
		  struct mem_cgroup *memcg)
{
	struct list_lru_node *nlru = &lru->node[nid];
	struct list_lru_one *l;

	l = lock_list_lru_of_memcg(lru, nid, memcg, false, false);
	if (!l)
		return false;
	if (list_empty(item)) {
		list_add_tail(item, &l->list);
		/* Set shrinker bit if the first element was added */
		if (!l->nr_items++)
			set_shrinker_bit(memcg, nid, lru_shrinker_id(lru));
		unlock_list_lru(l, false);
		atomic_long_inc(&nlru->nr_items);
		return true;
	}
	unlock_list_lru(l, false);
	return false;
}
EXPORT_SYMBOL_GPL(list_lru_add);

bool list_lru_add_obj(struct list_lru *lru, struct list_head *item)
{
	bool ret;
	int nid = page_to_nid(virt_to_page(item));

	if (list_lru_memcg_aware(lru)) {
		rcu_read_lock();
		ret = list_lru_add(lru, item, nid, mem_cgroup_from_virt(item));
		rcu_read_unlock();
	} else {
		ret = list_lru_add(lru, item, nid, NULL);
	}

	return ret;
}
EXPORT_SYMBOL_GPL(list_lru_add_obj);

/* The caller must ensure the memcg lifetime. */
bool list_lru_del(struct list_lru *lru, struct list_head *item, int nid,
		  struct mem_cgroup *memcg)
{
	struct list_lru_node *nlru = &lru->node[nid];
	struct list_lru_one *l;
	l = lock_list_lru_of_memcg(lru, nid, memcg, false, false);
	if (!l)
		return false;
	if (!list_empty(item)) {
		list_del_init(item);
		l->nr_items--;
		unlock_list_lru(l, false);
		atomic_long_dec(&nlru->nr_items);
		return true;
	}
	unlock_list_lru(l, false);
	return false;
}

bool list_lru_del_obj(struct list_lru *lru, struct list_head *item)
{
	bool ret;
	int nid = page_to_nid(virt_to_page(item));

	if (list_lru_memcg_aware(lru)) {
		rcu_read_lock();
		ret = list_lru_del(lru, item, nid, mem_cgroup_from_virt(item));
		rcu_read_unlock();
	} else {
		ret = list_lru_del(lru, item, nid, NULL);
	}

	return ret;
}
EXPORT_SYMBOL_GPL(list_lru_del_obj);

void list_lru_isolate(struct list_lru_one *list, struct list_head *item)
{
	list_del_init(item);
	list->nr_items--;
}
EXPORT_SYMBOL_GPL(list_lru_isolate);

void list_lru_isolate_move(struct list_lru_one *list, struct list_head *item,
			   struct list_head *head)
{
	list_move(item, head);
	list->nr_items--;
}
EXPORT_SYMBOL_GPL(list_lru_isolate_move);

unsigned long list_lru_count_one(struct list_lru *lru,
				 int nid, struct mem_cgroup *memcg)
{
	struct list_lru_one *l;
	long count;

	rcu_read_lock();
	l = list_lru_from_memcg_idx(lru, nid, memcg_kmem_id(memcg));
	count = l ? READ_ONCE(l->nr_items) : 0;
	rcu_read_unlock();

	if (unlikely(count < 0))
		count = 0;

	return count;
}
EXPORT_SYMBOL_GPL(list_lru_count_one);

unsigned long list_lru_count_node(struct list_lru *lru, int nid)
{
	struct list_lru_node *nlru;

	nlru = &lru->node[nid];
	return atomic_long_read(&nlru->nr_items);
}
EXPORT_SYMBOL_GPL(list_lru_count_node);

static unsigned long
__list_lru_walk_one(struct list_lru *lru, int nid, struct mem_cgroup *memcg,
		    list_lru_walk_cb isolate, void *cb_arg,
		    unsigned long *nr_to_walk, bool irq_off)
{
	struct list_lru_node *nlru = &lru->node[nid];
	struct list_lru_one *l = NULL;
	struct list_head *item, *n;
	unsigned long isolated = 0;

restart:
	l = lock_list_lru_of_memcg(lru, nid, memcg, irq_off, true);
	if (!l)
		return isolated;
	list_for_each_safe(item, n, &l->list) {
		enum lru_status ret;

		/*
		 * decrement nr_to_walk first so that we don't livelock if we
		 * get stuck on large numbers of LRU_RETRY items
		 */
		if (!*nr_to_walk)
			break;
		--*nr_to_walk;

		ret = isolate(item, l, cb_arg);
		switch (ret) {
		/*
		 * LRU_RETRY, LRU_REMOVED_RETRY and LRU_STOP will drop the lru
		 * lock. List traversal will have to restart from scratch.
		 */
		case LRU_RETRY:
			goto restart;
		case LRU_REMOVED_RETRY:
			fallthrough;
		case LRU_REMOVED:
			isolated++;
			atomic_long_dec(&nlru->nr_items);
			if (ret == LRU_REMOVED_RETRY)
				goto restart;
			break;
		case LRU_ROTATE:
			list_move_tail(item, &l->list);
			break;
		case LRU_SKIP:
			break;
		case LRU_STOP:
			goto out;
		default:
			BUG();
		}
	}
	unlock_list_lru(l, irq_off);
out:
	return isolated;
}

unsigned long
list_lru_walk_one(struct list_lru *lru, int nid, struct mem_cgroup *memcg,
		  list_lru_walk_cb isolate, void *cb_arg,
		  unsigned long *nr_to_walk)
{
	return __list_lru_walk_one(lru, nid, memcg, isolate,
				   cb_arg, nr_to_walk, false);
}
EXPORT_SYMBOL_GPL(list_lru_walk_one);

unsigned long
list_lru_walk_one_irq(struct list_lru *lru, int nid, struct mem_cgroup *memcg,
		      list_lru_walk_cb isolate, void *cb_arg,
		      unsigned long *nr_to_walk)
{
	return __list_lru_walk_one(lru, nid, memcg, isolate,
				   cb_arg, nr_to_walk, true);
}

unsigned long list_lru_walk_node(struct list_lru *lru, int nid,
				 list_lru_walk_cb isolate, void *cb_arg,
				 unsigned long *nr_to_walk)
{
	long isolated = 0;

	isolated += list_lru_walk_one(lru, nid, NULL, isolate, cb_arg,
				      nr_to_walk);


	return isolated;
}
EXPORT_SYMBOL_GPL(list_lru_walk_node);

static void init_one_lru(struct list_lru *lru, struct list_lru_one *l)
{
	INIT_LIST_HEAD(&l->list);
	spin_lock_init(&l->lock);
	l->nr_items = 0;
}

static inline void memcg_init_list_lru(struct list_lru *lru, bool memcg_aware)
{
}

static void memcg_destroy_list_lru(struct list_lru *lru)
{
}

int __list_lru_init(struct list_lru *lru, bool memcg_aware, struct shrinker *shrinker)
{
	int i;


	lru->node = kzalloc_objs(*lru->node, nr_node_ids);
	if (!lru->node)
		return -ENOMEM;

	for_each_node(i)
		init_one_lru(lru, &lru->node[i].lru);

	memcg_init_list_lru(lru, memcg_aware);
	list_lru_register(lru);

	return 0;
}
EXPORT_SYMBOL_GPL(__list_lru_init);

void list_lru_destroy(struct list_lru *lru)
{
	/* Already destroyed or not yet initialized? */
	if (!lru->node)
		return;

	list_lru_unregister(lru);

	memcg_destroy_list_lru(lru);
	kfree(lru->node);
	lru->node = NULL;

}
EXPORT_SYMBOL_GPL(list_lru_destroy);
