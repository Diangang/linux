// SPDX-License-Identifier: GPL-2.0
/*
 * i8253 PIT clocksource
 */
#include <linux/clockchips.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/spinlock.h>
#include <linux/timex.h>
#include <linux/module.h>
#include <linux/i8253.h>
#include <linux/smp.h>

/*
 * Protects access to I/O ports
 *
 * 0040-0043 : timer0, i8253 / i8254
 * 0061-0061 : NMI Control Register which contains two speaker control bits.
 */
DEFINE_RAW_SPINLOCK(i8253_lock);
EXPORT_SYMBOL(i8253_lock);

#ifdef CONFIG_CLKEVT_I8253
void clockevent_i8253_disable(void)
{
	guard(raw_spinlock_irqsave)(&i8253_lock);

	/*
	 * Writing the MODE register should stop the counter, according to
	 * the datasheet. This appears to work on real hardware (well, on
	 * modern Intel and AMD boxes; I didn't dig the Pegasos out of the
	 * shed).
	 *
	 * However, some virtual implementations differ, and the MODE change
	 * doesn't have any effect until either the counter is written (KVM
	 * in-kernel PIT) or the next interrupt (QEMU). And in those cases,
	 * it may not stop the *count*, only the interrupts. Although in
	 * the virt case, that probably doesn't matter, as the value of the
	 * counter will only be calculated on demand if the guest reads it;
	 * it's the interrupts which cause steal time.
	 *
	 * Hyper-V apparently has a bug where even in mode 0, the IRQ keeps
	 * firing repeatedly if the counter is running. But it *does* do the
	 * right thing when the MODE register is written.
	 *
	 * So: write the MODE and then load the counter, which ensures that
	 * the IRQ is stopped on those buggy virt implementations. And then
	 * write the MODE again, which is the right way to stop it.
	 */
	outb_p(0x30, PIT_MODE);
	outb_p(0, PIT_CH0);
	outb_p(0, PIT_CH0);

	outb_p(0x30, PIT_MODE);
}

static int pit_shutdown(struct clock_event_device *evt)
{
	if (!clockevent_state_oneshot(evt) && !clockevent_state_periodic(evt))
		return 0;

	clockevent_i8253_disable();
	return 0;
}

static int pit_set_oneshot(struct clock_event_device *evt)
{
	raw_spin_lock(&i8253_lock);
	outb_p(0x38, PIT_MODE);
	raw_spin_unlock(&i8253_lock);
	return 0;
}

static int pit_set_periodic(struct clock_event_device *evt)
{
	raw_spin_lock(&i8253_lock);

	/* binary, mode 2, LSB/MSB, ch 0 */
	outb_p(0x34, PIT_MODE);
	outb_p(PIT_LATCH & 0xff, PIT_CH0);	/* LSB */
	outb_p(PIT_LATCH >> 8, PIT_CH0);	/* MSB */

	raw_spin_unlock(&i8253_lock);
	return 0;
}

/*
 * Program the next event in oneshot mode
 *
 * Delta is given in PIT ticks
 */
static int pit_next_event(unsigned long delta, struct clock_event_device *evt)
{
	raw_spin_lock(&i8253_lock);
	outb_p(delta & 0xff , PIT_CH0);	/* LSB */
	outb_p(delta >> 8 , PIT_CH0);		/* MSB */
	raw_spin_unlock(&i8253_lock);

	return 0;
}

/*
 * On UP the PIT can serve all of the possible timer functions. On SMP systems
 * it can be solely used for the global tick.
 */
struct clock_event_device i8253_clockevent = {
	.name			= "pit",
	.features		= CLOCK_EVT_FEAT_PERIODIC,
	.set_state_shutdown	= pit_shutdown,
	.set_state_periodic	= pit_set_periodic,
	.set_next_event		= pit_next_event,
};

/*
 * Initialize the conversion factor and the min/max deltas of the clock event
 * structure and register the clock event source with the framework.
 */
void __init clockevent_i8253_init(bool oneshot)
{
	if (oneshot) {
		i8253_clockevent.features |= CLOCK_EVT_FEAT_ONESHOT;
		i8253_clockevent.set_state_oneshot = pit_set_oneshot;
	}
	/*
	 * Start pit with the boot cpu mask. x86 might make it global
	 * when it is used as broadcast device later.
	 */
	i8253_clockevent.cpumask = cpumask_of(smp_processor_id());

	clockevents_config_and_register(&i8253_clockevent, PIT_TICK_RATE,
					0xF, 0x7FFF);
}
#endif
