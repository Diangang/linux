/* SPDX-License-Identifier: GPL-2.0+ */
#ifndef _LINUX_OF_PRIVATE_H
#define _LINUX_OF_PRIVATE_H
/*
 * Private symbols used by OF support code
 *
 * Paul Mackerras	August 1996.
 * Copyright (C) 1996-2005 Paul Mackerras.
 */

#define FDT_ALIGN_SIZE 8
#define MAX_RESERVED_REGIONS    64

/**
 * struct alias_prop - Alias property in 'aliases' node
 * @link:	List node to link the structure in aliases_lookup list
 * @alias:	Alias property name
 * @np:		Pointer to device_node that the alias stands for
 * @id:		Index value from end of alias name
 * @stem:	Alias string without the index
 *
 * The structure represents one alias property of 'aliases' node as
 * an entry in aliases_lookup list.
 */
struct alias_prop {
	struct list_head link;
	const char *alias;
	struct device_node *np;
	int id;
	char stem[];
};

#if defined(CONFIG_SPARC)
#define OF_ROOT_NODE_ADDR_CELLS_DEFAULT 2
#else
#define OF_ROOT_NODE_ADDR_CELLS_DEFAULT 1
#endif

#define OF_ROOT_NODE_SIZE_CELLS_DEFAULT 1

extern struct mutex of_mutex;
extern raw_spinlock_t devtree_lock;
extern struct list_head aliases_lookup;

void __of_phandle_cache_inv_entry(phandle handle);

extern void *__unflatten_device_tree(const void *blob,
			      struct device_node *dad,
			      struct device_node **mynodes,
			      void *(*dt_alloc)(u64 size, u64 align),
			      bool detached);

void of_alias_scan(void * (*dt_alloc)(u64 size, u64 align));

/**
 * General utilities for working with live trees.
 *
 * All functions with two leading underscores operate
 * without taking node references, so you either have to
 * own the devtree lock or work on detached trees only.
 */
struct property *__of_prop_dup(const struct property *prop, gfp_t allocflags);
void __of_prop_free(struct property *prop);
struct device_node *__of_node_dup(const struct device_node *np,
				  const char *full_name);

struct device_node *__of_find_node_by_path(const struct device_node *parent,
						const char *path);
struct device_node *__of_find_node_by_full_path(struct device_node *node,
						const char *path);

extern const void *__of_get_property(const struct device_node *np,
				     const char *name, int *lenp);
/* illegal phandle value (set when unresolved) */
#define OF_PHANDLE_ILLEGAL	0xdeadbeef

extern int of_bus_n_addr_cells(struct device_node *np);
extern int of_bus_n_size_cells(struct device_node *np);

const __be32 *of_irq_parse_imap_parent(const __be32 *imap, int len,
				       struct of_phandle_args *out_irq);

struct bus_dma_region;
#if defined(CONFIG_OF_ADDRESS) && defined(CONFIG_HAS_DMA)
int of_dma_get_range(struct device_node *np,
		const struct bus_dma_region **map);
struct device_node *__of_get_dma_parent(const struct device_node *np);
#else
static inline int of_dma_get_range(struct device_node *np,
		const struct bus_dma_region **map)
{
	return -ENODEV;
}
static inline struct device_node *__of_get_dma_parent(const struct device_node *np)
{
	return of_get_parent(np);
}
#endif

int fdt_scan_reserved_mem(void);
void __init fdt_scan_reserved_mem_late(void);

bool of_fdt_device_is_available(const void *blob, unsigned long node);

/* Max address size we deal with */
#define OF_MAX_ADDR_CELLS	4
#define OF_CHECK_ADDR_COUNT(na)	((na) > 0 && (na) <= OF_MAX_ADDR_CELLS)
#define OF_CHECK_COUNTS(na, ns)	(OF_CHECK_ADDR_COUNT(na) && (ns) > 0)

/* Debug utility */
#ifdef DEBUG
static void __maybe_unused of_dump_addr(const char *s, const __be32 *addr, int na)
{
	pr_debug("%s", s);
	while (na--)
		pr_cont(" %08x", be32_to_cpu(*(addr++)));
	pr_cont("\n");
}
#else
static void __maybe_unused of_dump_addr(const char *s, const __be32 *addr, int na) { }
#endif

static inline bool is_pseudo_property(const char *prop_name)
{
	return !of_prop_cmp(prop_name, "name") ||
		!of_prop_cmp(prop_name, "phandle") ||
		!of_prop_cmp(prop_name, "linux,phandle");
}

#endif /* _LINUX_OF_PRIVATE_H */
