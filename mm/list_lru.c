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

static inline struct list_lru_one *
lock_list_lru(struct list_lru *lru, int nid, bool irq)
{
	struct list_lru_one *l = &lru->node[nid];

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

bool list_lru_add(struct list_lru *lru, struct list_head *item, int nid)
{
	struct list_lru_one *l;

	l = lock_list_lru(lru, nid, false);
	if (list_empty(item)) {
		list_add_tail(item, &l->list);
		l->nr_items++;
		unlock_list_lru(l, false);
		return true;
	}
	unlock_list_lru(l, false);
	return false;
}
EXPORT_SYMBOL_GPL(list_lru_add);

bool list_lru_add_obj(struct list_lru *lru, struct list_head *item)
{
	int nid = page_to_nid(virt_to_page(item));

	return list_lru_add(lru, item, nid);
}
EXPORT_SYMBOL_GPL(list_lru_add_obj);

bool list_lru_del(struct list_lru *lru, struct list_head *item, int nid)
{
	struct list_lru_one *l;
	l = lock_list_lru(lru, nid, false);
	if (!list_empty(item)) {
		list_del_init(item);
		l->nr_items--;
		unlock_list_lru(l, false);
		return true;
	}
	unlock_list_lru(l, false);
	return false;
}

bool list_lru_del_obj(struct list_lru *lru, struct list_head *item)
{
	int nid = page_to_nid(virt_to_page(item));

	return list_lru_del(lru, item, nid);
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

unsigned long list_lru_count_one(struct list_lru *lru, int nid)
{
	return READ_ONCE(lru->node[nid].nr_items);
}
EXPORT_SYMBOL_GPL(list_lru_count_one);

unsigned long list_lru_count_node(struct list_lru *lru, int nid)
{
	return list_lru_count_one(lru, nid);
}
EXPORT_SYMBOL_GPL(list_lru_count_node);

static unsigned long
__list_lru_walk_one(struct list_lru *lru, int nid,
			    list_lru_walk_cb isolate, void *cb_arg,
		    unsigned long *nr_to_walk, bool irq_off)
{
	struct list_lru_one *l = NULL;
	struct list_head *item, *n;
	unsigned long isolated = 0;

restart:
	l = lock_list_lru(lru, nid, irq_off);
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
list_lru_walk_one(struct list_lru *lru, int nid,
			  list_lru_walk_cb isolate, void *cb_arg,
		  unsigned long *nr_to_walk)
{
	return __list_lru_walk_one(lru, nid, isolate,
				   cb_arg, nr_to_walk, false);
}
EXPORT_SYMBOL_GPL(list_lru_walk_one);

unsigned long
list_lru_walk_one_irq(struct list_lru *lru, int nid,
			      list_lru_walk_cb isolate, void *cb_arg,
		      unsigned long *nr_to_walk)
{
	return __list_lru_walk_one(lru, nid, isolate,
				   cb_arg, nr_to_walk, true);
}

unsigned long list_lru_walk_node(struct list_lru *lru, int nid,
				 list_lru_walk_cb isolate, void *cb_arg,
				 unsigned long *nr_to_walk)
{
	return list_lru_walk_one(lru, nid, isolate, cb_arg, nr_to_walk);
}
EXPORT_SYMBOL_GPL(list_lru_walk_node);

static void init_one_lru(struct list_lru_one *l)
{
	INIT_LIST_HEAD(&l->list);
	spin_lock_init(&l->lock);
	l->nr_items = 0;
}

int list_lru_init(struct list_lru *lru)
{
	int i;

	lru->node = kzalloc_objs(*lru->node, nr_node_ids);
	if (!lru->node)
		return -ENOMEM;

	for_each_node(i)
		init_one_lru(&lru->node[i]);

	return 0;
}
EXPORT_SYMBOL_GPL(list_lru_init);

void list_lru_destroy(struct list_lru *lru)
{
	/* Already destroyed or not yet initialized? */
	if (!lru->node)
		return;

	kfree(lru->node);
	lru->node = NULL;
}
EXPORT_SYMBOL_GPL(list_lru_destroy);
