/* SPDX-License-Identifier: GPL-2.0 */
#ifndef LINUX_SSB_MIPSCORE_H_
#define LINUX_SSB_MIPSCORE_H_


struct ssb_mipscore {
};

static inline
void ssb_mipscore_init(struct ssb_mipscore *mcore)
{
}

static inline unsigned int ssb_mips_irq(struct ssb_device *dev)
{
	return 0;
}


#endif /* LINUX_SSB_MIPSCORE_H_ */
