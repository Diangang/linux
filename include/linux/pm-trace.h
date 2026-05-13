/* SPDX-License-Identifier: GPL-2.0 */
#ifndef PM_TRACE_H
#define PM_TRACE_H

#include <linux/types.h>

static inline bool pm_trace_rtc_valid(void) { return true; }
static inline int pm_trace_is_enabled(void) { return 0; }

#define TRACE_DEVICE(dev) do { } while (0)
#define TRACE_RESUME(dev) do { } while (0)
#define TRACE_SUSPEND(dev) do { } while (0)


#endif
