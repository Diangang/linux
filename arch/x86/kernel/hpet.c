// SPDX-License-Identifier: GPL-2.0-only
#include <linux/clockchips.h>
#include <linux/interrupt.h>
#include <linux/export.h>
#include <linux/delay.h>
#include <linux/hpet.h>
#include <linux/cpu.h>
#include <linux/irq.h>

#include <asm/cpuid/api.h>
#include <asm/irq_remapping.h>
#include <asm/hpet.h>
#include <asm/time.h>
#include <asm/mwait.h>
#include <asm/msr.h>

#undef  pr_fmt
#define pr_fmt(fmt) "hpet: " fmt

enum hpet_mode {
	HPET_MODE_UNUSED,
	HPET_MODE_LEGACY,
	HPET_MODE_CLOCKEVT,
	HPET_MODE_DEVICE,
};

struct hpet_channel {
	struct clock_event_device	evt;
	unsigned int			num;
	unsigned int			cpu;
	unsigned int			irq;
	unsigned int			in_use;
	enum hpet_mode			mode;
	unsigned int			boot_cfg;
	char				name[10];
};

struct hpet_base {
	unsigned int			nr_channels;
	unsigned int			nr_clockevents;
	unsigned int			boot_cfg;
	struct hpet_channel		*channels;
};

#define HPET_MASK			CLOCKSOURCE_MASK(32)

#define HPET_MIN_CYCLES			128
#define HPET_MIN_PROG_DELTA		(HPET_MIN_CYCLES + (HPET_MIN_CYCLES >> 1))

/*
 * HPET address is set in acpi/boot.c, when an ACPI entry exists
 */
unsigned long				hpet_address;
u8					hpet_blockid; /* OS timer block num */
bool					hpet_msi_disable;

#if defined(CONFIG_X86_LOCAL_APIC) && defined(CONFIG_GENERIC_MSI_IRQ)
static DEFINE_PER_CPU(struct hpet_channel *, cpu_hpet_channel);
static struct irq_domain		*hpet_domain;
#endif

static void __iomem			*hpet_virt_address;

static struct hpet_base			hpet_base;

static bool				hpet_legacy_int_enabled;
static unsigned long			hpet_freq;

bool					boot_hpet_disable;
bool					hpet_force_user;
static bool				hpet_verbose;

static inline
struct hpet_channel *clockevent_to_channel(struct clock_event_device *evt)
{
	return container_of(evt, struct hpet_channel, evt);
}

inline unsigned int hpet_readl(unsigned int a)
{
	return readl(hpet_virt_address + a);
}

static inline void hpet_writel(unsigned int d, unsigned int a)
{
	writel(d, hpet_virt_address + a);
}

static inline void hpet_set_mapping(void)
{
	hpet_virt_address = ioremap(hpet_address, HPET_MMAP_SIZE);
}

static inline void hpet_clear_mapping(void)
{
	iounmap(hpet_virt_address);
	hpet_virt_address = NULL;
}

/*
 * HPET command line enable / disable
 */
static int __init hpet_setup(char *str)
{
	while (str) {
		char *next = strchr(str, ',');

		if (next)
			*next++ = 0;
		if (!strncmp("disable", str, 7))
			boot_hpet_disable = true;
		if (!strncmp("force", str, 5))
			hpet_force_user = true;
		if (!strncmp("verbose", str, 7))
			hpet_verbose = true;
		str = next;
	}
	return 1;
}
__setup("hpet=", hpet_setup);

static int __init disable_hpet(char *str)
{
	boot_hpet_disable = true;
	return 1;
}
__setup("nohpet", disable_hpet);

static inline int is_hpet_capable(void)
{
	return !boot_hpet_disable && hpet_address;
}

/**
 * is_hpet_enabled - Check whether the legacy HPET timer interrupt is enabled
 */
int is_hpet_enabled(void)
{
	return is_hpet_capable() && hpet_legacy_int_enabled;
}
EXPORT_SYMBOL_GPL(is_hpet_enabled);

static void _hpet_print_config(const char *function, int line)
{
	u32 i, id, period, cfg, status, channels, l, h;

	pr_info("%s(%d):\n", function, line);

	id = hpet_readl(HPET_ID);
	period = hpet_readl(HPET_PERIOD);
	pr_info("ID: 0x%x, PERIOD: 0x%x\n", id, period);

	cfg = hpet_readl(HPET_CFG);
	status = hpet_readl(HPET_STATUS);
	pr_info("CFG: 0x%x, STATUS: 0x%x\n", cfg, status);

	l = hpet_readl(HPET_COUNTER);
	h = hpet_readl(HPET_COUNTER+4);
	pr_info("COUNTER_l: 0x%x, COUNTER_h: 0x%x\n", l, h);

	channels = ((id & HPET_ID_NUMBER) >> HPET_ID_NUMBER_SHIFT) + 1;

	for (i = 0; i < channels; i++) {
		l = hpet_readl(HPET_Tn_CFG(i));
		h = hpet_readl(HPET_Tn_CFG(i)+4);
		pr_info("T%d: CFG_l: 0x%x, CFG_h: 0x%x\n", i, l, h);

		l = hpet_readl(HPET_Tn_CMP(i));
		h = hpet_readl(HPET_Tn_CMP(i)+4);
		pr_info("T%d: CMP_l: 0x%x, CMP_h: 0x%x\n", i, l, h);

		l = hpet_readl(HPET_Tn_ROUTE(i));
		h = hpet_readl(HPET_Tn_ROUTE(i)+4);
		pr_info("T%d ROUTE_l: 0x%x, ROUTE_h: 0x%x\n", i, l, h);
	}
}

#define hpet_print_config()					\
do {								\
	if (hpet_verbose)					\
		_hpet_print_config(__func__, __LINE__);	\
} while (0)

/*
 * When the HPET driver (/dev/hpet) is enabled, we need to reserve
 * timer 0 and timer 1 in case of RTC emulation.
 */
static inline void hpet_reserve_platform_timers(void) { }
static inline void hpet_select_device_channel(void) {}

/* Common HPET functions */
static void hpet_stop_counter(void)
{
	u32 cfg = hpet_readl(HPET_CFG);

	cfg &= ~HPET_CFG_ENABLE;
	hpet_writel(cfg, HPET_CFG);
}

static void hpet_reset_counter(void)
{
	hpet_writel(0, HPET_COUNTER);
	hpet_writel(0, HPET_COUNTER + 4);
}

static void hpet_start_counter(void)
{
	unsigned int cfg = hpet_readl(HPET_CFG);

	cfg |= HPET_CFG_ENABLE;
	hpet_writel(cfg, HPET_CFG);
}

static void hpet_restart_counter(void)
{
	hpet_stop_counter();
	hpet_reset_counter();
	hpet_start_counter();
}

static void hpet_resume_device(void)
{
	force_hpet_resume();
}

static void hpet_resume_counter(struct clocksource *cs)
{
	hpet_resume_device();
	hpet_restart_counter();
}

static void hpet_enable_legacy_int(void)
{
	unsigned int cfg = hpet_readl(HPET_CFG);

	cfg |= HPET_CFG_LEGACY;
	hpet_writel(cfg, HPET_CFG);
	hpet_legacy_int_enabled = true;
}

static int hpet_clkevt_set_state_periodic(struct clock_event_device *evt)
{
	unsigned int channel = clockevent_to_channel(evt)->num;
	unsigned int cfg, cmp, now;
	uint64_t delta;

	hpet_stop_counter();
	delta = ((uint64_t)(NSEC_PER_SEC / HZ)) * evt->mult;
	delta >>= evt->shift;
	now = hpet_readl(HPET_COUNTER);
	cmp = now + (unsigned int)delta;
	cfg = hpet_readl(HPET_Tn_CFG(channel));
	cfg |= HPET_TN_ENABLE | HPET_TN_PERIODIC | HPET_TN_SETVAL |
	       HPET_TN_32BIT;
	hpet_writel(cfg, HPET_Tn_CFG(channel));
	hpet_writel(cmp, HPET_Tn_CMP(channel));
	udelay(1);
	/*
	 * HPET on AMD 81xx needs a second write (with HPET_TN_SETVAL
	 * cleared) to T0_CMP to set the period. The HPET_TN_SETVAL
	 * bit is automatically cleared after the first write.
	 * (See AMD-8111 HyperTransport I/O Hub Data Sheet,
	 * Publication # 24674)
	 */
	hpet_writel((unsigned int)delta, HPET_Tn_CMP(channel));
	hpet_start_counter();
	hpet_print_config();

	return 0;
}

static int hpet_clkevt_set_state_oneshot(struct clock_event_device *evt)
{
	unsigned int channel = clockevent_to_channel(evt)->num;
	unsigned int cfg;

	cfg = hpet_readl(HPET_Tn_CFG(channel));
	cfg &= ~HPET_TN_PERIODIC;
	cfg |= HPET_TN_ENABLE | HPET_TN_32BIT;
	hpet_writel(cfg, HPET_Tn_CFG(channel));

	return 0;
}

static int hpet_clkevt_set_state_shutdown(struct clock_event_device *evt)
{
	unsigned int channel = clockevent_to_channel(evt)->num;
	unsigned int cfg;

	cfg = hpet_readl(HPET_Tn_CFG(channel));
	cfg &= ~HPET_TN_ENABLE;
	hpet_writel(cfg, HPET_Tn_CFG(channel));

	return 0;
}

static int hpet_clkevt_legacy_resume(struct clock_event_device *evt)
{
	hpet_enable_legacy_int();
	hpet_print_config();
	return 0;
}

static int
hpet_clkevt_set_next_event(unsigned long delta, struct clock_event_device *evt)
{
	unsigned int channel = clockevent_to_channel(evt)->num;
	u32 cnt;
	s32 res;

	cnt = hpet_readl(HPET_COUNTER);
	cnt += (u32) delta;
	hpet_writel(cnt, HPET_Tn_CMP(channel));

	/*
	 * HPETs are a complete disaster. The compare register is
	 * based on a equal comparison and neither provides a less
	 * than or equal functionality (which would require to take
	 * the wraparound into account) nor a simple count down event
	 * mode. Further the write to the comparator register is
	 * delayed internally up to two HPET clock cycles in certain
	 * chipsets (ATI, ICH9,10). Some newer AMD chipsets have even
	 * longer delays. We worked around that by reading back the
	 * compare register, but that required another workaround for
	 * ICH9,10 chips where the first readout after write can
	 * return the old stale value. We already had a minimum
	 * programming delta of 5us enforced, but a NMI or SMI hitting
	 * between the counter readout and the comparator write can
	 * move us behind that point easily. Now instead of reading
	 * the compare register back several times, we make the ETIME
	 * decision based on the following: Return ETIME if the
	 * counter value after the write is less than HPET_MIN_CYCLES
	 * away from the event or if the counter is already ahead of
	 * the event. The minimum programming delta for the generic
	 * clockevents code is set to 1.5 * HPET_MIN_CYCLES.
	 */
	res = (s32)(cnt - hpet_readl(HPET_COUNTER));

	return res < HPET_MIN_CYCLES ? -ETIME : 0;
}

static void hpet_init_clockevent(struct hpet_channel *hc, unsigned int rating)
{
	struct clock_event_device *evt = &hc->evt;

	evt->rating		= rating;
	evt->irq		= hc->irq;
	evt->name		= hc->name;
	evt->cpumask		= cpumask_of(hc->cpu);
	evt->set_state_oneshot	= hpet_clkevt_set_state_oneshot;
	evt->set_next_event	= hpet_clkevt_set_next_event;
	evt->set_state_shutdown	= hpet_clkevt_set_state_shutdown;

	evt->features = CLOCK_EVT_FEAT_ONESHOT;
	if (hc->boot_cfg & HPET_TN_PERIODIC) {
		evt->features		|= CLOCK_EVT_FEAT_PERIODIC;
		evt->set_state_periodic	= hpet_clkevt_set_state_periodic;
	}
}

static void __init hpet_legacy_clockevent_register(struct hpet_channel *hc)
{
	/*
	 * Start HPET with the boot CPU's cpumask and make it global after
	 * the IO_APIC has been initialized.
	 */
	hc->cpu = boot_cpu_data.cpu_index;
	strscpy(hc->name, "hpet", sizeof(hc->name));
	hpet_init_clockevent(hc, 50);

	hc->evt.tick_resume	= hpet_clkevt_legacy_resume;

	/*
	 * Legacy horrors and sins from the past. HPET used periodic mode
	 * unconditionally forever on the legacy channel 0. Removing the
	 * below hack and using the conditional in hpet_init_clockevent()
	 * makes at least Qemu and one hardware machine fail to boot.
	 * There are two issues which cause the boot failure:
	 *
	 * #1 After the timer delivery test in IOAPIC and the IOAPIC setup
	 *    the next interrupt is not delivered despite the HPET channel
	 *    being programmed correctly. Reprogramming the HPET after
	 *    switching to IOAPIC makes it work again. After fixing this,
	 *    the next issue surfaces:
	 *
	 * #2 Due to the unconditional periodic mode availability the Local
	 *    APIC timer calibration can hijack the global clockevents
	 *    event handler without causing damage. Using oneshot at this
	 *    stage makes if hang because the HPET does not get
	 *    reprogrammed due to the handler hijacking. Duh, stupid me!
	 *
	 * Both issues require major surgery and especially the kick HPET
	 * again after enabling IOAPIC results in really nasty hackery.
	 * This 'assume periodic works' magic has survived since HPET
	 * support got added, so it's questionable whether this should be
	 * fixed. Both Qemu and the failing hardware machine support
	 * periodic mode despite the fact that both don't advertise it in
	 * the configuration register and both need that extra kick after
	 * switching to IOAPIC. Seems to be a feature...
	 */
	hc->evt.features		|= CLOCK_EVT_FEAT_PERIODIC;
	hc->evt.set_state_periodic	= hpet_clkevt_set_state_periodic;

	/* Start HPET legacy interrupts */
	hpet_enable_legacy_int();

	clockevents_config_and_register(&hc->evt, hpet_freq,
					HPET_MIN_PROG_DELTA, 0x7FFFFFFF);
	global_clock_event = &hc->evt;
	pr_debug("Clockevent registered\n");
}

/*
 * HPET MSI Support
 */
#if defined(CONFIG_X86_LOCAL_APIC) && defined(CONFIG_GENERIC_MSI_IRQ)
static void hpet_msi_unmask(struct irq_data *data)
{
	struct hpet_channel *hc = irq_data_get_irq_handler_data(data);
	unsigned int cfg;

	cfg = hpet_readl(HPET_Tn_CFG(hc->num));
	cfg |= HPET_TN_ENABLE | HPET_TN_FSB;
	hpet_writel(cfg, HPET_Tn_CFG(hc->num));
}

static void hpet_msi_mask(struct irq_data *data)
{
	struct hpet_channel *hc = irq_data_get_irq_handler_data(data);
	unsigned int cfg;

	cfg = hpet_readl(HPET_Tn_CFG(hc->num));
	cfg &= ~(HPET_TN_ENABLE | HPET_TN_FSB);
	hpet_writel(cfg, HPET_Tn_CFG(hc->num));
}

static void hpet_msi_write(struct hpet_channel *hc, struct msi_msg *msg)
{
	hpet_writel(msg->data, HPET_Tn_ROUTE(hc->num));
	hpet_writel(msg->address_lo, HPET_Tn_ROUTE(hc->num) + 4);
}

static void hpet_msi_write_msg(struct irq_data *data, struct msi_msg *msg)
{
	hpet_msi_write(irq_data_get_irq_handler_data(data), msg);
}

static struct irq_chip hpet_msi_controller __ro_after_init = {
	.name = "HPET-MSI",
	.irq_unmask = hpet_msi_unmask,
	.irq_mask = hpet_msi_mask,
	.irq_ack = irq_chip_ack_parent,
	.irq_set_affinity = msi_domain_set_affinity,
	.irq_retrigger = irq_chip_retrigger_hierarchy,
	.irq_write_msi_msg = hpet_msi_write_msg,
	.flags = IRQCHIP_SKIP_SET_WAKE | IRQCHIP_AFFINITY_PRE_STARTUP,
};

static int hpet_msi_init(struct irq_domain *domain,
			 struct msi_domain_info *info, unsigned int virq,
			 irq_hw_number_t hwirq, msi_alloc_info_t *arg)
{
	irq_domain_set_info(domain, virq, arg->hwirq, info->chip, NULL,
			    handle_edge_irq, arg->data, "edge");

	return 0;
}

static struct msi_domain_ops hpet_msi_domain_ops = {
	.msi_init	= hpet_msi_init,
};

static struct msi_domain_info hpet_msi_domain_info = {
	.ops		= &hpet_msi_domain_ops,
	.chip		= &hpet_msi_controller,
	.flags		= MSI_FLAG_USE_DEF_DOM_OPS,
};

static struct irq_domain *hpet_create_irq_domain(int hpet_id)
{
	struct msi_domain_info *domain_info;
	struct irq_domain *parent, *d;
	struct fwnode_handle *fn;
	struct irq_fwspec fwspec;

	if (x86_vector_domain == NULL)
		return NULL;

	domain_info = kzalloc_obj(*domain_info);
	if (!domain_info)
		return NULL;

	*domain_info = hpet_msi_domain_info;
	domain_info->data = (void *)(long)hpet_id;

	fn = irq_domain_alloc_named_id_fwnode(hpet_msi_controller.name,
					      hpet_id);
	if (!fn) {
		kfree(domain_info);
		return NULL;
	}

	fwspec.fwnode = fn;
	fwspec.param_count = 1;
	fwspec.param[0] = hpet_id;

	parent = irq_find_matching_fwspec(&fwspec, DOMAIN_BUS_GENERIC_MSI);
	if (!parent) {
		irq_domain_free_fwnode(fn);
		kfree(domain_info);
		return NULL;
	}
	if (parent != x86_vector_domain)
		hpet_msi_controller.name = "IR-HPET-MSI";

	d = msi_create_irq_domain(fn, domain_info, parent);
	if (!d) {
		irq_domain_free_fwnode(fn);
		kfree(domain_info);
	}
	return d;
}

static inline int hpet_dev_id(struct irq_domain *domain)
{
	struct msi_domain_info *info = msi_get_domain_info(domain);

	return (int)(long)info->data;
}

static int hpet_assign_irq(struct irq_domain *domain, struct hpet_channel *hc,
			   int dev_num)
{
	struct irq_alloc_info info;

	init_irq_alloc_info(&info, NULL);
	info.type = X86_IRQ_ALLOC_TYPE_HPET;
	info.data = hc;
	info.devid = hpet_dev_id(domain);
	info.hwirq = dev_num;

	return irq_domain_alloc_irqs(domain, 1, NUMA_NO_NODE, &info);
}

static int hpet_clkevt_msi_resume(struct clock_event_device *evt)
{
	struct hpet_channel *hc = clockevent_to_channel(evt);
	struct irq_data *data = irq_get_irq_data(hc->irq);
	struct msi_msg msg;

	/* Restore the MSI msg and unmask the interrupt */
	irq_chip_compose_msi_msg(data, &msg);
	hpet_msi_write(hc, &msg);
	hpet_msi_unmask(data);
	return 0;
}

static irqreturn_t hpet_msi_interrupt_handler(int irq, void *data)
{
	struct hpet_channel *hc = data;
	struct clock_event_device *evt = &hc->evt;

	if (!evt->event_handler) {
		pr_info("Spurious interrupt HPET channel %d\n", hc->num);
		return IRQ_HANDLED;
	}

	evt->event_handler(evt);
	return IRQ_HANDLED;
}

static int hpet_setup_msi_irq(struct hpet_channel *hc)
{
	if (request_irq(hc->irq, hpet_msi_interrupt_handler,
			IRQF_TIMER | IRQF_NOBALANCING,
			hc->name, hc))
		return -1;

	disable_irq(hc->irq);
	irq_set_affinity(hc->irq, cpumask_of(hc->cpu));
	enable_irq(hc->irq);

	pr_debug("%s irq %u for MSI\n", hc->name, hc->irq);

	return 0;
}

/* Invoked from the hotplug callback on @cpu */
static void init_one_hpet_msi_clockevent(struct hpet_channel *hc, int cpu)
{
	struct clock_event_device *evt = &hc->evt;

	hc->cpu = cpu;
	per_cpu(cpu_hpet_channel, cpu) = hc;
	hpet_setup_msi_irq(hc);

	hpet_init_clockevent(hc, 110);
	evt->tick_resume = hpet_clkevt_msi_resume;

	clockevents_config_and_register(evt, hpet_freq, HPET_MIN_PROG_DELTA,
					0x7FFFFFFF);
}

static struct hpet_channel *hpet_get_unused_clockevent(void)
{
	int i;

	for (i = 0; i < hpet_base.nr_channels; i++) {
		struct hpet_channel *hc = hpet_base.channels + i;

		if (hc->mode != HPET_MODE_CLOCKEVT || hc->in_use)
			continue;
		hc->in_use = 1;
		return hc;
	}
	return NULL;
}

static int hpet_cpuhp_online(unsigned int cpu)
{
	struct hpet_channel *hc = hpet_get_unused_clockevent();

	if (hc)
		init_one_hpet_msi_clockevent(hc, cpu);
	return 0;
}

static int hpet_cpuhp_dead(unsigned int cpu)
{
	struct hpet_channel *hc = per_cpu(cpu_hpet_channel, cpu);

	if (!hc)
		return 0;
	free_irq(hc->irq, hc);
	hc->in_use = 0;
	per_cpu(cpu_hpet_channel, cpu) = NULL;
	return 0;
}

static void __init hpet_select_clockevents(void)
{
	unsigned int i;

	hpet_base.nr_clockevents = 0;

	/* No point if MSI is disabled or CPU has an Always Running APIC Timer */
	if (hpet_msi_disable || boot_cpu_has(X86_FEATURE_ARAT))
		return;

	hpet_print_config();

	hpet_domain = hpet_create_irq_domain(hpet_blockid);
	if (!hpet_domain)
		return;

	for (i = 0; i < hpet_base.nr_channels; i++) {
		struct hpet_channel *hc = hpet_base.channels + i;
		int irq;

		if (hc->mode != HPET_MODE_UNUSED)
			continue;

		/* Only consider HPET channel with MSI support */
		if (!(hc->boot_cfg & HPET_TN_FSB_CAP))
			continue;

		sprintf(hc->name, "hpet%d", i);

		irq = hpet_assign_irq(hpet_domain, hc, hc->num);
		if (irq <= 0)
			continue;

		hc->irq = irq;
		hc->mode = HPET_MODE_CLOCKEVT;

		if (++hpet_base.nr_clockevents == num_possible_cpus())
			break;
	}

	pr_info("%d channels of %d reserved for per-cpu timers\n",
		hpet_base.nr_channels, hpet_base.nr_clockevents);
}

#else

static inline void hpet_select_clockevents(void) { }

#define hpet_cpuhp_online	NULL
#define hpet_cpuhp_dead		NULL

#endif

/*
 * Clock source related code
 */
#if defined(CONFIG_SMP) && defined(CONFIG_64BIT)
/*
 * Reading the HPET counter is a very slow operation. If a large number of
 * CPUs are trying to access the HPET counter simultaneously, it can cause
 * massive delays and slow down system performance dramatically. This may
 * happen when HPET is the default clock source instead of TSC. For a
 * really large system with hundreds of CPUs, the slowdown may be so
 * severe, that it can actually crash the system because of a NMI watchdog
 * soft lockup, for example.
 *
 * If multiple CPUs are trying to access the HPET counter at the same time,
 * we don't actually need to read the counter multiple times. Instead, the
 * other CPUs can use the counter value read by the first CPU in the group.
 *
 * This special feature is only enabled on x86-64 systems. It is unlikely
 * that 32-bit x86 systems will have enough CPUs to require this feature
 * with its associated locking overhead. We also need 64-bit atomic read.
 *
 * The lock and the HPET value are stored together and can be read in a
 * single atomic 64-bit read. It is explicitly assumed that arch_spinlock_t
 * is 32 bits in size.
 */
union hpet_lock {
	struct {
		arch_spinlock_t lock;
		u32 value;
	};
	u64 lockval;
};

static union hpet_lock hpet __cacheline_aligned = {
	{ .lock = __ARCH_SPIN_LOCK_UNLOCKED, },
};

static u64 read_hpet(struct clocksource *cs)
{
	unsigned long flags;
	union hpet_lock old, new;

	BUILD_BUG_ON(sizeof(union hpet_lock) != 8);

	/*
	 * Read HPET directly if in NMI.
	 */
	if (in_nmi())
		return (u64)hpet_readl(HPET_COUNTER);

	/*
	 * Read the current state of the lock and HPET value atomically.
	 */
	old.lockval = READ_ONCE(hpet.lockval);

	if (arch_spin_is_locked(&old.lock))
		goto contended;

	local_irq_save(flags);
	if (arch_spin_trylock(&hpet.lock)) {
		new.value = hpet_readl(HPET_COUNTER);
		/*
		 * Use WRITE_ONCE() to prevent store tearing.
		 */
		WRITE_ONCE(hpet.value, new.value);
		arch_spin_unlock(&hpet.lock);
		local_irq_restore(flags);
		return (u64)new.value;
	}
	local_irq_restore(flags);

contended:
	/*
	 * Contended case
	 * --------------
	 * Wait until the HPET value change or the lock is free to indicate
	 * its value is up-to-date.
	 *
	 * It is possible that old.value has already contained the latest
	 * HPET value while the lock holder was in the process of releasing
	 * the lock. Checking for lock state change will enable us to return
	 * the value immediately instead of waiting for the next HPET reader
	 * to come along.
	 */
	do {
		cpu_relax();
		new.lockval = READ_ONCE(hpet.lockval);
	} while ((new.value == old.value) && arch_spin_is_locked(&new.lock));

	return (u64)new.value;
}
#else
/*
 * For UP or 32-bit.
 */
static u64 read_hpet(struct clocksource *cs)
{
	return (u64)hpet_readl(HPET_COUNTER);
}
#endif

static struct clocksource clocksource_hpet = {
	.name		= "hpet",
	.rating		= 250,
	.read		= read_hpet,
	.mask		= HPET_MASK,
	.flags		= CLOCK_SOURCE_IS_CONTINUOUS | CLOCK_SOURCE_CALIBRATED,
	.resume		= hpet_resume_counter,
};

/*
 * AMD SB700 based systems with spread spectrum enabled use a SMM based
 * HPET emulation to provide proper frequency setting.
 *
 * On such systems the SMM code is initialized with the first HPET register
 * access and takes some time to complete. During this time the config
 * register reads 0xffffffff. We check for max 1000 loops whether the
 * config register reads a non-0xffffffff value to make sure that the
 * HPET is up and running before we proceed any further.
 *
 * A counting loop is safe, as the HPET access takes thousands of CPU cycles.
 *
 * On non-SB700 based machines this check is only done once and has no
 * side effects.
 */
static bool __init hpet_cfg_working(void)
{
	int i;

	for (i = 0; i < 1000; i++) {
		if (hpet_readl(HPET_CFG) != 0xFFFFFFFF)
			return true;
	}

	pr_warn("Config register invalid. Disabling HPET\n");
	return false;
}

static bool __init hpet_counting(void)
{
	u64 start, now, t1;

	hpet_restart_counter();

	t1 = hpet_readl(HPET_COUNTER);
	start = rdtsc();

	/*
	 * We don't know the TSC frequency yet, but waiting for
	 * 200000 TSC cycles is safe:
	 * 4 GHz == 50us
	 * 1 GHz == 200us
	 */
	do {
		if (t1 != hpet_readl(HPET_COUNTER))
			return true;
		now = rdtsc();
	} while ((now - start) < 200000UL);

	pr_warn("Counter not counting. HPET disabled\n");
	return false;
}

static bool __init mwait_pc10_supported(void)
{
	unsigned int eax, ebx, ecx, mwait_substates;

	if (boot_cpu_data.x86_vendor != X86_VENDOR_INTEL)
		return false;

	if (!cpu_feature_enabled(X86_FEATURE_MWAIT))
		return false;

	cpuid(CPUID_LEAF_MWAIT, &eax, &ebx, &ecx, &mwait_substates);

	return (ecx & CPUID5_ECX_EXTENSIONS_SUPPORTED) &&
	       (ecx & CPUID5_ECX_INTERRUPT_BREAK) &&
	       (mwait_substates & (0xF << 28));
}

/*
 * Check whether the system supports PC10. If so force disable HPET as that
 * stops counting in PC10. This check is overbroad as it does not take any
 * of the following into account:
 *
 *	- ACPI tables
 *	- Enablement of intel_idle
 *	- Command line arguments which limit intel_idle C-state support
 *
 * That's perfectly fine. HPET is a piece of hardware designed by committee
 * and the only reasons why it is still in use on modern systems is the
 * fact that it is impossible to reliably query TSC and CPU frequency via
 * CPUID or firmware.
 *
 * If HPET is functional it is useful for calibrating TSC, but this can be
 * done via PMTIMER as well which seems to be the last remaining timer on
 * X86/INTEL platforms that has not been completely wreckaged by feature
 * creep.
 *
 * In theory HPET support should be removed altogether, but there are older
 * systems out there which depend on it because TSC and APIC timer are
 * dysfunctional in deeper C-states.
 *
 * It's only 20 years now that hardware people have been asked to provide
 * reliable and discoverable facilities which can be used for timekeeping
 * and per CPU timer interrupts.
 *
 * The probability that this problem is going to be solved in the
 * foreseeable future is close to zero, so the kernel has to be cluttered
 * with heuristics to keep up with the ever growing amount of hardware and
 * firmware trainwrecks. Hopefully some day hardware people will understand
 * that the approach of "This can be fixed in software" is not sustainable.
 * Hope dies last...
 */
static bool __init hpet_is_pc10_damaged(void)
{
	unsigned long long pcfg;

	/* Check whether PC10 substates are supported */
	if (!mwait_pc10_supported())
		return false;

	/* Check whether PC10 is enabled in PKG C-state limit */
	rdmsrq(MSR_PKG_CST_CONFIG_CONTROL, pcfg);
	if ((pcfg & 0xF) < 8)
		return false;

	if (hpet_force_user) {
		pr_warn("HPET force enabled via command line, but dysfunctional in PC10.\n");
		return false;
	}

	pr_info("HPET dysfunctional in PC10. Force disabled.\n");
	boot_hpet_disable = true;
	return true;
}

/**
 * hpet_enable - Try to setup the HPET timer. Returns 1 on success.
 */
int __init hpet_enable(void)
{
	u32 hpet_period, cfg, id, irq;
	unsigned int i, channels;
	struct hpet_channel *hc;
	u64 freq;

	if (!is_hpet_capable())
		return 0;

	if (hpet_is_pc10_damaged())
		return 0;

	hpet_set_mapping();
	if (!hpet_virt_address)
		return 0;

	/* Validate that the config register is working */
	if (!hpet_cfg_working())
		goto out_nohpet;

	/*
	 * Read the period and check for a sane value:
	 */
	hpet_period = hpet_readl(HPET_PERIOD);
	if (hpet_period < HPET_MIN_PERIOD || hpet_period > HPET_MAX_PERIOD)
		goto out_nohpet;

	/* The period is a femtoseconds value. Convert it to a frequency. */
	freq = FSEC_PER_SEC;
	do_div(freq, hpet_period);
	hpet_freq = freq;

	/*
	 * Read the HPET ID register to retrieve the IRQ routing
	 * information and the number of channels
	 */
	id = hpet_readl(HPET_ID);
	hpet_print_config();

	/* This is the HPET channel number which is zero based */
	channels = ((id & HPET_ID_NUMBER) >> HPET_ID_NUMBER_SHIFT) + 1;

	hc = kzalloc_objs(*hc, channels);
	if (!hc) {
		pr_warn("Disabling HPET.\n");
		goto out_nohpet;
	}
	hpet_base.channels = hc;
	hpet_base.nr_channels = channels;

	/* Read, store and sanitize the global configuration */
	cfg = hpet_readl(HPET_CFG);
	hpet_base.boot_cfg = cfg;
	cfg &= ~(HPET_CFG_ENABLE | HPET_CFG_LEGACY);
	hpet_writel(cfg, HPET_CFG);
	if (cfg)
		pr_warn("Global config: Unknown bits %#x\n", cfg);

	/* Read, store and sanitize the per channel configuration */
	for (i = 0; i < channels; i++, hc++) {
		hc->num = i;

		cfg = hpet_readl(HPET_Tn_CFG(i));
		hc->boot_cfg = cfg;
		irq = (cfg & Tn_INT_ROUTE_CNF_MASK) >> Tn_INT_ROUTE_CNF_SHIFT;
		hc->irq = irq;

		cfg &= ~(HPET_TN_ENABLE | HPET_TN_LEVEL | HPET_TN_FSB);
		hpet_writel(cfg, HPET_Tn_CFG(i));

		cfg &= ~(HPET_TN_PERIODIC | HPET_TN_PERIODIC_CAP
			 | HPET_TN_64BIT_CAP | HPET_TN_32BIT | HPET_TN_ROUTE
			 | HPET_TN_FSB | HPET_TN_FSB_CAP);
		if (cfg)
			pr_warn("Channel #%u config: Unknown bits %#x\n", i, cfg);
	}
	hpet_print_config();

	/*
	 * Validate that the counter is counting. This needs to be done
	 * after sanitizing the config registers to properly deal with
	 * force enabled HPETs.
	 */
	if (!hpet_counting())
		goto out_nohpet;

	clocksource_register_hz(&clocksource_hpet, (u32)hpet_freq);

	if (id & HPET_ID_LEGSUP) {
		hpet_legacy_clockevent_register(&hpet_base.channels[0]);
		hpet_base.channels[0].mode = HPET_MODE_LEGACY;
		return 1;
	}
	return 0;

out_nohpet:
	kfree(hpet_base.channels);
	hpet_base.channels = NULL;
	hpet_base.nr_channels = 0;
	hpet_clear_mapping();
	hpet_address = 0;
	return 0;
}

/*
 * The late initialization runs after the PCI quirks have been invoked
 * which might have detected a system on which the HPET can be enforced.
 *
 * Also, the MSI machinery is not working yet when the HPET is initialized
 * early.
 *
 * If the HPET is enabled, then:
 *
 *  1) Reserve one channel for /dev/hpet if CONFIG_HPET=y
 *  2) Reserve up to num_possible_cpus() channels as per CPU clockevents
 *  3) Setup /dev/hpet if CONFIG_HPET=y
 *  4) Register hotplug callbacks when clockevents are available
 */
static __init int hpet_late_init(void)
{
	int ret;

	if (!hpet_address) {
		if (!force_hpet_address)
			return -ENODEV;

		hpet_address = force_hpet_address;
		hpet_enable();
	}

	if (!hpet_virt_address)
		return -ENODEV;

	hpet_select_device_channel();
	hpet_select_clockevents();
	hpet_reserve_platform_timers();
	hpet_print_config();

	if (!hpet_base.nr_clockevents)
		return 0;

	ret = cpuhp_setup_state(CPUHP_AP_X86_HPET_ONLINE, "x86/hpet:online",
				hpet_cpuhp_online, NULL);
	if (ret)
		return ret;
	ret = cpuhp_setup_state(CPUHP_X86_HPET_DEAD, "x86/hpet:dead", NULL,
				hpet_cpuhp_dead);
	if (ret)
		goto err_cpuhp;
	return 0;

err_cpuhp:
	cpuhp_remove_state(CPUHP_AP_X86_HPET_ONLINE);
	return ret;
}
fs_initcall(hpet_late_init);

void hpet_disable(void)
{
	unsigned int i;
	u32 cfg;

	if (!is_hpet_capable() || !hpet_virt_address)
		return;

	/* Restore boot configuration with the enable bit cleared */
	cfg = hpet_base.boot_cfg;
	cfg &= ~HPET_CFG_ENABLE;
	hpet_writel(cfg, HPET_CFG);

	/* Restore the channel boot configuration */
	for (i = 0; i < hpet_base.nr_channels; i++)
		hpet_writel(hpet_base.channels[i].boot_cfg, HPET_Tn_CFG(i));

	/* If the HPET was enabled at boot time, reenable it */
	if (hpet_base.boot_cfg & HPET_CFG_ENABLE)
		hpet_writel(hpet_base.boot_cfg, HPET_CFG);
}
