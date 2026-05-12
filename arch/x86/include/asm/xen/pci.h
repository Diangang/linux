/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_X86_XEN_PCI_H
#define _ASM_X86_XEN_PCI_H

#define pci_xen 0
#define pci_xen_init (0)
static inline int pci_xen_hvm_init(void)
{
	return -1;
}

#ifdef CONFIG_XEN_PV_DOM0
int __init pci_xen_initial_domain(void);
#else
static inline int __init pci_xen_initial_domain(void)
{
	return -1;
}
#endif


#endif	/* _ASM_X86_XEN_PCI_H */
