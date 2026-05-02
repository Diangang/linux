// SPDX-License-Identifier: GPL-2.0
/*
 * RTC related functions
 */
#include <linux/acpi.h>
#include <linux/platform_device.h>
#include <linux/mc146818rtc.h>
#include <linux/export.h>

#include <asm/vsyscall.h>
#include <asm/x86_init.h>
#include <asm/time.h>
#include <asm/setup.h>

#ifdef CONFIG_X86_32
/*
 * This is a special lock that is owned by the CPU and holds the index
 * register we are working with.  It is required for NMI access to the
 * CMOS/RTC registers.  See arch/x86/include/asm/mc146818rtc.h for details.
 */
volatile unsigned long cmos_lock;
EXPORT_SYMBOL(cmos_lock);
#endif /* CONFIG_X86_32 */

DEFINE_SPINLOCK(rtc_lock);
EXPORT_SYMBOL(rtc_lock);

/* Routines for accessing the CMOS RAM/RTC. */
unsigned char rtc_cmos_read(unsigned char addr)
{
	unsigned char val;

	lock_cmos_prefix(addr);
	outb(addr, RTC_PORT(0));
	val = inb(RTC_PORT(1));
	lock_cmos_suffix(addr);

	return val;
}
EXPORT_SYMBOL(rtc_cmos_read);

void rtc_cmos_write(unsigned char val, unsigned char addr)
{
	lock_cmos_prefix(addr);
	outb(addr, RTC_PORT(0));
	outb(val, RTC_PORT(1));
	lock_cmos_suffix(addr);
}
EXPORT_SYMBOL(rtc_cmos_write);

/* not static: needed by APM */
void read_persistent_clock64(struct timespec64 *ts)
{
	x86_platform.get_wallclock(ts);
}


static struct resource rtc_resources[] = {
	[0] = {
		.start	= RTC_PORT(0),
		.end	= RTC_PORT(1),
		.flags	= IORESOURCE_IO,
	},
	[1] = {
		.start	= RTC_IRQ,
		.end	= RTC_IRQ,
		.flags	= IORESOURCE_IRQ,
	}
};

static struct platform_device rtc_device = {
	.name		= "rtc_cmos",
	.id		= -1,
	.resource	= rtc_resources,
	.num_resources	= ARRAY_SIZE(rtc_resources),
};

static __init int add_rtc_cmos(void)
{
	if (cmos_rtc_platform_device_present)
		return 0;

	if (!x86_platform.legacy.rtc)
		return -ENODEV;

	platform_device_register(&rtc_device);
	dev_info(&rtc_device.dev, "registered fallback platform RTC device\n");

	return 0;
}
device_initcall(add_rtc_cmos);
