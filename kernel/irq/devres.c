// SPDX-License-Identifier: GPL-2.0
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/irqdomain.h>
#include <linux/device.h>
#include <linux/gfp.h>
#include <linux/irq.h>

#include "internals.h"

/*
 * Device resource management aware IRQ request/free implementation.
 */
struct irq_devres {
	unsigned int irq;
	void *dev_id;
};

static void devm_irq_release(struct device *dev, void *res)
{
	struct irq_devres *this = res;

	free_irq(this->irq, this->dev_id);
}

static int devm_irq_match(struct device *dev, void *res, void *data)
{
	struct irq_devres *this = res, *match = data;

	return this->irq == match->irq && this->dev_id == match->dev_id;
}

static int devm_request_result(struct device *dev, int rc, unsigned int irq,
			       irq_handler_t handler, irq_handler_t thread_fn,
			       const char *devname)
{
	if (rc >= 0)
		return rc;

	return dev_err_probe(dev, rc, "request_irq(%u) %ps %ps %s\n",
			     irq, handler, thread_fn, devname ? : "");
}

static int __devm_request_threaded_irq(struct device *dev, unsigned int irq,
				       irq_handler_t handler,
				       irq_handler_t thread_fn,
				       unsigned long irqflags,
				       const char *devname, void *dev_id)
{
	struct irq_devres *dr;
	int rc;

	dr = devres_alloc(devm_irq_release, sizeof(struct irq_devres),
			  GFP_KERNEL);
	if (!dr)
		return -ENOMEM;

	if (!devname)
		devname = dev_name(dev);

	rc = request_threaded_irq(irq, handler, thread_fn, irqflags, devname,
				  dev_id);
	if (rc) {
		devres_free(dr);
		return rc;
	}

	dr->irq = irq;
	dr->dev_id = dev_id;
	devres_add(dev, dr);

	return 0;
}

/**
 * devm_request_threaded_irq - allocate an interrupt line for a managed device with error logging
 * @dev:	Device to request interrupt for
 * @irq:	Interrupt line to allocate
 * @handler:	Function to be called when the interrupt occurs
 * @thread_fn:	Function to be called in a threaded interrupt context. NULL
 *		for devices which handle everything in @handler
 * @irqflags:	Interrupt type flags
 * @devname:	An ascii name for the claiming device, dev_name(dev) if NULL
 * @dev_id:	A cookie passed back to the handler function
 *
 * Except for the extra @dev argument, this function takes the same
 * arguments and performs the same function as request_threaded_irq().
 * Interrupts requested with this function will be automatically freed on
 * driver detach.
 *
 * If an interrupt allocated with this function needs to be freed
 * separately, devm_free_irq() must be used.
 *
 * When the request fails, an error message is printed with contextual
 * information (device name, interrupt number, handler functions and
 * error code). Don't add extra error messages at the call sites.
 *
 * Return: 0 on success or a negative error number.
 */
int devm_request_threaded_irq(struct device *dev, unsigned int irq,
			      irq_handler_t handler, irq_handler_t thread_fn,
			      unsigned long irqflags, const char *devname,
			      void *dev_id)
{
	int rc = __devm_request_threaded_irq(dev, irq, handler, thread_fn,
					     irqflags, devname, dev_id);

	return devm_request_result(dev, rc, irq, handler, thread_fn, devname);
}
EXPORT_SYMBOL(devm_request_threaded_irq);

static int __devm_request_any_context_irq(struct device *dev, unsigned int irq,
					  irq_handler_t handler,
					  unsigned long irqflags,
					  const char *devname, void *dev_id)
{
	struct irq_devres *dr;
	int rc;

	dr = devres_alloc(devm_irq_release, sizeof(struct irq_devres),
			  GFP_KERNEL);
	if (!dr)
		return -ENOMEM;

	if (!devname)
		devname = dev_name(dev);

	rc = request_any_context_irq(irq, handler, irqflags, devname, dev_id);
	if (rc < 0) {
		devres_free(dr);
		return rc;
	}

	dr->irq = irq;
	dr->dev_id = dev_id;
	devres_add(dev, dr);

	return rc;
}

/**
 * devm_request_any_context_irq - allocate an interrupt line for a managed device with error logging
 * @dev:	Device to request interrupt for
 * @irq:	Interrupt line to allocate
 * @handler:	Function to be called when the interrupt occurs
 * @irqflags:	Interrupt type flags
 * @devname:	An ascii name for the claiming device, dev_name(dev) if NULL
 * @dev_id:	A cookie passed back to the handler function
 *
 * Except for the extra @dev argument, this function takes the same
 * arguments and performs the same function as request_any_context_irq().
 * Interrupts requested with this function will be automatically freed on
 * driver detach.
 *
 * If an interrupt allocated with this function needs to be freed
 * separately, devm_free_irq() must be used.
 *
 * When the request fails, an error message is printed with contextual
 * information (device name, interrupt number, handler functions and
 * error code). Don't add extra error messages at the call sites.
 *
 * Return: IRQC_IS_HARDIRQ or IRQC_IS_NESTED on success, or a negative error
 * number.
 */
int devm_request_any_context_irq(struct device *dev, unsigned int irq,
				 irq_handler_t handler, unsigned long irqflags,
				 const char *devname, void *dev_id)
{
	int rc = __devm_request_any_context_irq(dev, irq, handler, irqflags,
						devname, dev_id);

	return devm_request_result(dev, rc, irq, handler, NULL, devname);
}
EXPORT_SYMBOL(devm_request_any_context_irq);

/**
 *	devm_free_irq - free an interrupt
 *	@dev: device to free interrupt for
 *	@irq: Interrupt line to free
 *	@dev_id: Device identity to free
 *
 *	Except for the extra @dev argument, this function takes the
 *	same arguments and performs the same function as free_irq().
 *	This function instead of free_irq() should be used to manually
 *	free IRQs allocated with devm_request_irq().
 */
void devm_free_irq(struct device *dev, unsigned int irq, void *dev_id)
{
	struct irq_devres match_data = { irq, dev_id };

	WARN_ON(devres_release(dev, devm_irq_release, devm_irq_match,
			       &match_data));
}
EXPORT_SYMBOL(devm_free_irq);

struct irq_desc_devres {
	unsigned int from;
	unsigned int cnt;
};

static void devm_irq_desc_release(struct device *dev, void *res)
{
	struct irq_desc_devres *this = res;

	irq_free_descs(this->from, this->cnt);
}

/**
 * __devm_irq_alloc_descs - Allocate and initialize a range of irq descriptors
 *			    for a managed device
 * @dev:	Device to allocate the descriptors for
 * @irq:	Allocate for specific irq number if irq >= 0
 * @from:	Start the search from this irq number
 * @cnt:	Number of consecutive irqs to allocate
 * @node:	Preferred node on which the irq descriptor should be allocated
 * @owner:	Owning module (can be NULL)
 * @affinity:	Optional pointer to an irq_affinity_desc array of size @cnt
 *		which hints where the irq descriptors should be allocated
 *		and which default affinities to use
 *
 * Returns the first irq number or error code.
 *
 * Note: Use the provided wrappers (devm_irq_alloc_desc*) for simplicity.
 */
int __devm_irq_alloc_descs(struct device *dev, int irq, unsigned int from,
			   unsigned int cnt, int node, struct module *owner,
			   const struct irq_affinity_desc *affinity)
{
	struct irq_desc_devres *dr;
	int base;

	dr = devres_alloc(devm_irq_desc_release, sizeof(*dr), GFP_KERNEL);
	if (!dr)
		return -ENOMEM;

	base = __irq_alloc_descs(irq, from, cnt, node, owner, affinity);
	if (base < 0) {
		devres_free(dr);
		return base;
	}

	dr->from = base;
	dr->cnt = cnt;
	devres_add(dev, dr);

	return base;
}
EXPORT_SYMBOL_GPL(__devm_irq_alloc_descs);


#ifdef CONFIG_IRQ_DOMAIN
static void devm_irq_domain_remove(struct device *dev, void *res)
{
	struct irq_domain **domain = res;

	irq_domain_remove(*domain);
}

/**
 * devm_irq_domain_instantiate() - Instantiate a new irq domain data for a
 *                                 managed device.
 * @dev:	Device to instantiate the domain for
 * @info:	Domain information pointer pointing to the information for this
 *		domain
 *
 * Return: A pointer to the instantiated irq domain or an ERR_PTR value.
 */
struct irq_domain *devm_irq_domain_instantiate(struct device *dev,
					       const struct irq_domain_info *info)
{
	struct irq_domain *domain;
	struct irq_domain **dr;

	dr = devres_alloc(devm_irq_domain_remove, sizeof(*dr), GFP_KERNEL);
	if (!dr)
		return ERR_PTR(-ENOMEM);

	domain = irq_domain_instantiate(info);
	if (!IS_ERR(domain)) {
		*dr = domain;
		devres_add(dev, dr);
	} else {
		devres_free(dr);
	}

	return domain;
}
EXPORT_SYMBOL_GPL(devm_irq_domain_instantiate);
#endif /* CONFIG_IRQ_DOMAIN */
