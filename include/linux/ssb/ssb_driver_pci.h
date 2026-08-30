/* SPDX-License-Identifier: GPL-2.0 */
#ifndef LINUX_SSB_PCICORE_H_
#define LINUX_SSB_PCICORE_H_

#include <linux/types.h>

struct pci_dev;




struct ssb_pcicore {
};

static inline
void ssb_pcicore_init(struct ssb_pcicore *pc)
{
}

static inline
int ssb_pcicore_dev_irqvecs_enable(struct ssb_pcicore *pc,
				   struct ssb_device *dev)
{
	return 0;
}

static inline
int ssb_pcicore_plat_dev_init(struct pci_dev *d)
{
	return -ENODEV;
}
static inline
int ssb_pcicore_pcibios_map_irq(const struct pci_dev *dev, u8 slot, u8 pin)
{
	return -ENODEV;
}

#endif /* LINUX_SSB_PCICORE_H_ */
