// SPDX-License-Identifier: GPL-2.0-only
/*
 * ksyms_common.c: A split of kernel/kallsyms.c
 * Contains a few generic function definations independent of config KALLSYMS.
 */
#include <linux/kallsyms.h>
#include <linux/security.h>

/*
 * We show kallsyms information even to normal users if we've enabled
 * kernel profiling and kptr_restrict is clear. Otherwise, require
 * CAP_SYSLOG (assuming kptr_restrict isn't set to
 * block even that).
 */
bool kallsyms_show_value(const struct cred *cred)
{
	switch (kptr_restrict) {
	case 0:
	case 1:
		if (security_capable(cred, &init_user_ns, CAP_SYSLOG,
				     CAP_OPT_NOAUDIT) == 0)
			return true;
		fallthrough;
	default:
		return false;
	}
}
