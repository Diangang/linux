// SPDX-License-Identifier: GPL-2.0
/*
 * Basic Node interface support
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/memory.h>
#include <linux/mempolicy.h>
#include <linux/vmstat.h>
#include <linux/notifier.h>
#include <linux/node.h>
#include <linux/hugetlb.h>
#include <linux/compaction.h>
#include <linux/cpumask.h>
#include <linux/topology.h>
#include <linux/nodemask.h>
#include <linux/cpu.h>
#include <linux/device.h>
#include <linux/pm_runtime.h>
#include <linux/swap.h>
#include <linux/slab.h>
#include <linux/memblock.h>

static const struct bus_type node_subsys = {
	.name = "node",
	.dev_name = "node",
};

static inline ssize_t cpumap_read(struct file *file, struct kobject *kobj,
				  const struct bin_attribute *attr, char *buf,
				  loff_t off, size_t count)
{
	struct device *dev = kobj_to_dev(kobj);
	struct node *node_dev = to_node(dev);
	cpumask_var_t mask;
	ssize_t n;

	if (!alloc_cpumask_var(&mask, GFP_KERNEL))
		return 0;

	cpumask_and(mask, cpumask_of_node(node_dev->dev.id), cpu_online_mask);
	n = cpumap_print_bitmask_to_buf(buf, mask, off, count);
	free_cpumask_var(mask);

	return n;
}

static const BIN_ATTR_RO(cpumap, CPUMAP_FILE_MAX_BYTES);

static inline ssize_t cpulist_read(struct file *file, struct kobject *kobj,
				   const struct bin_attribute *attr, char *buf,
				   loff_t off, size_t count)
{
	struct device *dev = kobj_to_dev(kobj);
	struct node *node_dev = to_node(dev);
	cpumask_var_t mask;
	ssize_t n;

	if (!alloc_cpumask_var(&mask, GFP_KERNEL))
		return 0;

	cpumask_and(mask, cpumask_of_node(node_dev->dev.id), cpu_online_mask);
	n = cpumap_print_list_to_buf(buf, mask, off, count);
	free_cpumask_var(mask);

	return n;
}

static const BIN_ATTR_RO(cpulist, CPULIST_FILE_MAX_BYTES);

/**
 * struct node_access_nodes - Access class device to hold user visible
 * 			      relationships to other nodes.
 * @dev:	Device for this memory access class
 * @list_node:	List element in the node's access list
 * @access:	The access class rank
 * @coord:	Heterogeneous memory performance coordinates
 */
struct node_access_nodes {
	struct device		dev;
	struct list_head	list_node;
	unsigned int		access;
};
#define to_access_nodes(dev) container_of(dev, struct node_access_nodes, dev)

static struct attribute *node_init_access_node_attrs[] = {
	NULL,
};

static struct attribute *node_targ_access_node_attrs[] = {
	NULL,
};

static const struct attribute_group initiators = {
	.name	= "initiators",
	.attrs	= node_init_access_node_attrs,
};

static const struct attribute_group targets = {
	.name	= "targets",
	.attrs	= node_targ_access_node_attrs,
};

static const struct attribute_group *node_access_node_groups[] = {
	&initiators,
	&targets,
	NULL,
};


static void node_remove_accesses(struct node *node)
{
	struct node_access_nodes *c, *cnext;

	list_for_each_entry_safe(c, cnext, &node->access_list, list_node) {
		list_del(&c->list_node);
		device_unregister(&c->dev);
	}
}

static void node_access_release(struct device *dev)
{
	kfree(to_access_nodes(dev));
}

static struct node_access_nodes *node_init_node_access(struct node *node,
						       enum access_coordinate_class access)
{
	struct node_access_nodes *access_node;
	struct device *dev;

	list_for_each_entry(access_node, &node->access_list, list_node)
		if (access_node->access == access)
			return access_node;

	access_node = kzalloc_obj(*access_node);
	if (!access_node)
		return NULL;

	access_node->access = access;
	dev = &access_node->dev;
	dev->parent = &node->dev;
	dev->release = node_access_release;
	dev->groups = node_access_node_groups;
	if (dev_set_name(dev, "access%u", access))
		goto free;

	if (device_register(dev))
		goto free_name;

	list_add_tail(&access_node->list_node, &node->access_list);
	return access_node;
free_name:
	kfree_const(dev->kobj.name);
free:
	kfree(access_node);
	return NULL;
}

static void node_init_caches(unsigned int nid) { }
static void node_remove_caches(struct node *node) { }

#define K(x) ((x) << (PAGE_SHIFT - 10))
static ssize_t node_read_meminfo(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	int len = 0;
	int nid = dev->id;
	struct pglist_data *pgdat = NODE_DATA(nid);
	struct sysinfo i;
	unsigned long sreclaimable, sunreclaimable;
	unsigned long swapcached = 0;

	si_meminfo_node(&i, nid);
	sreclaimable = node_page_state_pages(pgdat, NR_SLAB_RECLAIMABLE_B);
	sunreclaimable = node_page_state_pages(pgdat, NR_SLAB_UNRECLAIMABLE_B);
	len = sysfs_emit_at(buf, len,
			    "Node %d MemTotal:       %8lu kB\n"
			    "Node %d MemFree:        %8lu kB\n"
			    "Node %d MemUsed:        %8lu kB\n"
			    "Node %d SwapCached:     %8lu kB\n"
			    "Node %d Active:         %8lu kB\n"
			    "Node %d Inactive:       %8lu kB\n"
			    "Node %d Active(anon):   %8lu kB\n"
			    "Node %d Inactive(anon): %8lu kB\n"
			    "Node %d Active(file):   %8lu kB\n"
			    "Node %d Inactive(file): %8lu kB\n"
			    "Node %d Unevictable:    %8lu kB\n"
			    "Node %d Mlocked:        %8lu kB\n",
			    nid, K(i.totalram),
			    nid, K(i.freeram),
			    nid, K(i.totalram - i.freeram),
			    nid, K(swapcached),
			    nid, K(node_page_state(pgdat, NR_ACTIVE_ANON) +
				   node_page_state(pgdat, NR_ACTIVE_FILE)),
			    nid, K(node_page_state(pgdat, NR_INACTIVE_ANON) +
				   node_page_state(pgdat, NR_INACTIVE_FILE)),
			    nid, K(node_page_state(pgdat, NR_ACTIVE_ANON)),
			    nid, K(node_page_state(pgdat, NR_INACTIVE_ANON)),
			    nid, K(node_page_state(pgdat, NR_ACTIVE_FILE)),
			    nid, K(node_page_state(pgdat, NR_INACTIVE_FILE)),
			    nid, K(node_page_state(pgdat, NR_UNEVICTABLE)),
			    nid, K(sum_zone_node_page_state(nid, NR_MLOCK)));

	len += sysfs_emit_at(buf, len,
			     "Node %d Dirty:          %8lu kB\n"
			     "Node %d Writeback:      %8lu kB\n"
			     "Node %d FilePages:      %8lu kB\n"
			     "Node %d Mapped:         %8lu kB\n"
			     "Node %d AnonPages:      %8lu kB\n"
			     "Node %d Shmem:          %8lu kB\n"
			     "Node %d KernelStack:    %8lu kB\n"
			     "Node %d PageTables:     %8lu kB\n"
			     "Node %d SecPageTables:  %8lu kB\n"
			     "Node %d NFS_Unstable:   %8lu kB\n"
			     "Node %d Bounce:         %8lu kB\n"
			     "Node %d WritebackTmp:   %8lu kB\n"
			     "Node %d KReclaimable:   %8lu kB\n"
			     "Node %d Slab:           %8lu kB\n"
			     "Node %d SReclaimable:   %8lu kB\n"
			     "Node %d SUnreclaim:     %8lu kB\n"
			     "Node %d GPUActive:      %8lu kB\n"
			     "Node %d GPUReclaim:     %8lu kB\n"
			     ,
			     nid, K(node_page_state(pgdat, NR_FILE_DIRTY)),
			     nid, K(node_page_state(pgdat, NR_WRITEBACK)),
			     nid, K(node_page_state(pgdat, NR_FILE_PAGES)),
			     nid, K(node_page_state(pgdat, NR_FILE_MAPPED)),
			     nid, K(node_page_state(pgdat, NR_ANON_MAPPED)),
			     nid, K(i.sharedram),
			     nid, node_page_state(pgdat, NR_KERNEL_STACK_KB),
			     nid, K(node_page_state(pgdat, NR_PAGETABLE)),
			     nid, K(node_page_state(pgdat, NR_SECONDARY_PAGETABLE)),
			     nid, 0UL,
			     nid, 0UL,
			     nid, 0UL,
			     nid, K(sreclaimable +
				    node_page_state(pgdat, NR_KERNEL_MISC_RECLAIMABLE)),
			     nid, K(sreclaimable + sunreclaimable),
			     nid, K(sreclaimable),
			     nid, K(sunreclaimable)
			     ,
			     nid, K(node_page_state(pgdat, NR_GPU_ACTIVE)),
			     nid, K(node_page_state(pgdat, NR_GPU_RECLAIM))
			    );
	len += hugetlb_report_node_meminfo(buf, len, nid);
	return len;
}

#undef K
static DEVICE_ATTR(meminfo, 0444, node_read_meminfo, NULL);

static ssize_t node_read_numastat(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	fold_vm_numa_events();
	return sysfs_emit(buf,
			  "numa_hit %lu\n"
			  "numa_miss %lu\n"
			  "numa_foreign %lu\n"
			  "interleave_hit %lu\n"
			  "local_node %lu\n"
			  "other_node %lu\n",
			  sum_zone_numa_event_state(dev->id, NUMA_HIT),
			  sum_zone_numa_event_state(dev->id, NUMA_MISS),
			  sum_zone_numa_event_state(dev->id, NUMA_FOREIGN),
			  sum_zone_numa_event_state(dev->id, NUMA_INTERLEAVE_HIT),
			  sum_zone_numa_event_state(dev->id, NUMA_LOCAL),
			  sum_zone_numa_event_state(dev->id, NUMA_OTHER));
}
static DEVICE_ATTR(numastat, 0444, node_read_numastat, NULL);

static ssize_t node_read_vmstat(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int nid = dev->id;
	struct pglist_data *pgdat = NODE_DATA(nid);
	int i;
	int len = 0;

	for (i = 0; i < NR_VM_ZONE_STAT_ITEMS; i++)
		len += sysfs_emit_at(buf, len, "%s %lu\n",
				     zone_stat_name(i),
				     sum_zone_node_page_state(nid, i));

#ifdef CONFIG_NUMA
	fold_vm_numa_events();
	for (i = 0; i < NR_VM_NUMA_EVENT_ITEMS; i++)
		len += sysfs_emit_at(buf, len, "%s %lu\n",
				     numa_stat_name(i),
				     sum_zone_numa_event_state(nid, i));

#endif
	for (i = 0; i < NR_VM_NODE_STAT_ITEMS; i++) {
		unsigned long pages = node_page_state_pages(pgdat, i);

		if (vmstat_item_print_in_thp(i))
			pages /= HPAGE_PMD_NR;
		len += sysfs_emit_at(buf, len, "%s %lu\n", node_stat_name(i),
				     pages);
	}

	return len;
}
static DEVICE_ATTR(vmstat, 0444, node_read_vmstat, NULL);

static ssize_t node_read_distance(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	int nid = dev->id;
	int len = 0;
	int i;

	/*
	 * buf is currently PAGE_SIZE in length and each node needs 4 chars
	 * at the most (distance + space or newline).
	 */
	BUILD_BUG_ON(MAX_NUMNODES * 4 > PAGE_SIZE);

	for_each_online_node(i) {
		len += sysfs_emit_at(buf, len, "%s%d",
				     i ? " " : "", node_distance(nid, i));
	}

	len += sysfs_emit_at(buf, len, "\n");
	return len;
}
static DEVICE_ATTR(distance, 0444, node_read_distance, NULL);

static struct attribute *node_dev_attrs[] = {
	&dev_attr_meminfo.attr,
	&dev_attr_numastat.attr,
	&dev_attr_distance.attr,
	&dev_attr_vmstat.attr,
	NULL
};

static const struct bin_attribute *node_dev_bin_attrs[] = {
	&bin_attr_cpumap,
	&bin_attr_cpulist,
	NULL
};

static const struct attribute_group node_dev_group = {
	.attrs = node_dev_attrs,
	.bin_attrs = node_dev_bin_attrs,
};

static const struct attribute_group *node_dev_groups[] = {
	&node_dev_group,
#ifdef CONFIG_MEMORY_FAILURE
	&memory_failure_attr_group,
#endif
	NULL
};

static void node_device_release(struct device *dev)
{
	kfree(to_node(dev));
}

struct node *node_devices[MAX_NUMNODES];

/*
 * register cpu under node
 */
int register_cpu_under_node(unsigned int cpu, unsigned int nid)
{
	int ret;
	struct device *obj;

	if (!node_online(nid))
		return 0;

	obj = get_cpu_device(cpu);
	if (!obj)
		return 0;

	ret = sysfs_create_link(&node_devices[nid]->dev.kobj,
				&obj->kobj,
				kobject_name(&obj->kobj));
	if (ret)
		return ret;

	return sysfs_create_link(&obj->kobj,
				 &node_devices[nid]->dev.kobj,
				 kobject_name(&node_devices[nid]->dev.kobj));
}

/**
 * register_memory_node_under_compute_node - link memory node to its compute
 *					     node for a given access class.
 * @mem_nid:	Memory node number
 * @cpu_nid:	Cpu  node number
 * @access:	Access class to register
 *
 * Description:
 * 	For use with platforms that may have separate memory and compute nodes.
 * 	This function will export node relationships linking which memory
 * 	initiator nodes can access memory targets at a given ranked access
 * 	class.
 */
int register_memory_node_under_compute_node(unsigned int mem_nid,
					    unsigned int cpu_nid,
					    enum access_coordinate_class access)
{
	struct node *init_node, *targ_node;
	struct node_access_nodes *initiator, *target;
	int ret;

	if (!node_online(cpu_nid) || !node_online(mem_nid))
		return -ENODEV;

	init_node = node_devices[cpu_nid];
	targ_node = node_devices[mem_nid];
	initiator = node_init_node_access(init_node, access);
	target = node_init_node_access(targ_node, access);
	if (!initiator || !target)
		return -ENOMEM;

	ret = sysfs_add_link_to_group(&initiator->dev.kobj, "targets",
				      &targ_node->dev.kobj,
				      dev_name(&targ_node->dev));
	if (ret)
		return ret;

	ret = sysfs_add_link_to_group(&target->dev.kobj, "initiators",
				      &init_node->dev.kobj,
				      dev_name(&init_node->dev));
	if (ret)
		goto err;

	return 0;
 err:
	sysfs_remove_link_from_group(&initiator->dev.kobj, "targets",
				     dev_name(&targ_node->dev));
	return ret;
}

int unregister_cpu_under_node(unsigned int cpu, unsigned int nid)
{
	struct device *obj;

	if (!node_online(nid))
		return 0;

	obj = get_cpu_device(cpu);
	if (!obj)
		return 0;

	sysfs_remove_link(&node_devices[nid]->dev.kobj,
			  kobject_name(&obj->kobj));
	sysfs_remove_link(&obj->kobj,
			  kobject_name(&node_devices[nid]->dev.kobj));

	return 0;
}


/**
 * register_node - Initialize and register the node device.
 * @nid: Node number to use when creating the device.
 *
 * Return: 0 on success, -errno otherwise
 */
int register_node(int nid)
{
	int error;
	int cpu;
	struct node *node;

	node = kzalloc_obj(struct node);
	if (!node)
		return -ENOMEM;

	INIT_LIST_HEAD(&node->access_list);

	node->dev.id = nid;
	node->dev.bus = &node_subsys;
	node->dev.release = node_device_release;
	node->dev.groups = node_dev_groups;

	error = device_register(&node->dev);
	if (error) {
		put_device(&node->dev);
		return error;
	}

	node_devices[nid] = node;
	hugetlb_register_node(node);
	compaction_register_node(node);
	reclaim_register_node(node);

	/* link cpu under this node */
	for_each_present_cpu(cpu) {
		if (cpu_to_node(cpu) == nid)
			register_cpu_under_node(cpu, nid);
	}

	node_init_caches(nid);

	return error;
}
/**
 * unregister_node - unregister a node device
 * @nid: nid of the node going away
 *
 * Unregisters the node device at node id @nid. All the devices on the
 * node must be unregistered before calling this function.
 */
void unregister_node(int nid)
{
	struct node *node = node_devices[nid];

	if (!node)
		return;

	hugetlb_unregister_node(node);
	compaction_unregister_node(node);
	reclaim_unregister_node(node);
	node_remove_accesses(node);
	node_remove_caches(node);
	device_unregister(&node->dev);
	node_devices[nid] = NULL;
}

/*
 * node states attributes
 */

struct node_attr {
	struct device_attribute attr;
	enum node_states state;
};

static ssize_t show_node_state(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	struct node_attr *na = container_of(attr, struct node_attr, attr);

	return sysfs_emit(buf, "%*pbl\n",
			  nodemask_pr_args(&node_states[na->state]));
}

#define _NODE_ATTR(name, state) \
	{ __ATTR(name, 0444, show_node_state, NULL), state }

static struct node_attr node_state_attr[] = {
	[N_POSSIBLE] = _NODE_ATTR(possible, N_POSSIBLE),
	[N_ONLINE] = _NODE_ATTR(online, N_ONLINE),
	[N_NORMAL_MEMORY] = _NODE_ATTR(has_normal_memory, N_NORMAL_MEMORY),
	[N_MEMORY] = _NODE_ATTR(has_memory, N_MEMORY),
	[N_CPU] = _NODE_ATTR(has_cpu, N_CPU),
	[N_GENERIC_INITIATOR] = _NODE_ATTR(has_generic_initiator,
					   N_GENERIC_INITIATOR),
};

static struct attribute *node_state_attrs[] = {
	&node_state_attr[N_POSSIBLE].attr.attr,
	&node_state_attr[N_ONLINE].attr.attr,
	&node_state_attr[N_NORMAL_MEMORY].attr.attr,
	&node_state_attr[N_MEMORY].attr.attr,
	&node_state_attr[N_CPU].attr.attr,
	&node_state_attr[N_GENERIC_INITIATOR].attr.attr,
	NULL
};

static const struct attribute_group memory_root_attr_group = {
	.attrs = node_state_attrs,
};

static const struct attribute_group *cpu_root_attr_groups[] = {
	&memory_root_attr_group,
	NULL,
};

void __init node_dev_init(void)
{
	int ret, i;

 	BUILD_BUG_ON(ARRAY_SIZE(node_state_attr) != NR_NODE_STATES);
 	BUILD_BUG_ON(ARRAY_SIZE(node_state_attrs)-1 != NR_NODE_STATES);

	ret = subsys_system_register(&node_subsys, cpu_root_attr_groups);
	if (ret)
		panic("%s() failed to register subsystem: %d\n", __func__, ret);

	/*
	 * Create all node devices, which will properly link the node
	 * to already created cpu devices.
	 */
	for_each_online_node(i) {
		ret =  register_node(i);
		if (ret)
			panic("%s() failed to add node: %d\n", __func__, ret);
	}

	register_memory_blocks_under_nodes();
}
