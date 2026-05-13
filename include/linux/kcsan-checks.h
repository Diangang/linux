/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_KCSAN_CHECKS_H
#define _LINUX_KCSAN_CHECKS_H

#include <linux/compiler_attributes.h>
#include <linux/types.h>

#define KCSAN_ACCESS_WRITE	(1 << 0)
#define KCSAN_ACCESS_COMPOUND	(1 << 1)
#define KCSAN_ACCESS_ATOMIC	(1 << 2)
#define KCSAN_ACCESS_ASSERT	(1 << 3)
#define KCSAN_ACCESS_SCOPED	(1 << 4)

static inline void __kcsan_check_access(const volatile void *ptr, size_t size,
					int type) { }
static inline void __kcsan_mb(void) { }
static inline void __kcsan_wmb(void) { }
static inline void __kcsan_rmb(void) { }
static inline void __kcsan_release(void) { }
static inline void kcsan_disable_current(void) { }
static inline void kcsan_enable_current(void) { }
static inline void kcsan_enable_current_nowarn(void) { }
static inline void kcsan_nestable_atomic_begin(void) { }
static inline void kcsan_nestable_atomic_end(void) { }
static inline void kcsan_flat_atomic_begin(void) { }
static inline void kcsan_flat_atomic_end(void) { }
static inline void kcsan_atomic_next(int n) { }
static inline void kcsan_set_access_mask(unsigned long mask) { }

struct kcsan_scoped_access { };
#define __kcsan_cleanup_scoped __maybe_unused
static inline struct kcsan_scoped_access *
kcsan_begin_scoped_access(const volatile void *ptr, size_t size, int type,
			  struct kcsan_scoped_access *sa)
{
	return sa;
}
static inline void kcsan_end_scoped_access(struct kcsan_scoped_access *sa) { }

static inline void kcsan_check_access(const volatile void *ptr, size_t size,
				      int type) { }
static inline void kcsan_check_read(const volatile void *ptr, size_t size) { }
static inline void kcsan_check_write(const volatile void *ptr, size_t size) { }
static inline void kcsan_check_read_write(const volatile void *ptr, size_t size) { }
static inline void kcsan_check_atomic_read(const volatile void *ptr, size_t size) { }
static inline void kcsan_check_atomic_write(const volatile void *ptr, size_t size) { }
static inline void kcsan_check_atomic_read_write(const volatile void *ptr, size_t size) { }
static inline void __kcsan_enable_current(void) { }
static inline void __kcsan_disable_current(void) { }

#define __KCSAN_BARRIER_TO_SIGNAL_FENCE_mb	__ATOMIC_SEQ_CST
#define __KCSAN_BARRIER_TO_SIGNAL_FENCE_wmb	__ATOMIC_ACQ_REL
#define __KCSAN_BARRIER_TO_SIGNAL_FENCE_rmb	__ATOMIC_ACQUIRE
#define __KCSAN_BARRIER_TO_SIGNAL_FENCE_release	__ATOMIC_RELEASE
#define __kcsan_mb()
#define __kcsan_wmb()
#define __kcsan_rmb()
#define __kcsan_release()
#define kcsan_mb()
#define kcsan_wmb()
#define kcsan_rmb()
#define kcsan_release()
#define ASSERT_EXCLUSIVE_WRITER(var) do { } while (0)
#define ASSERT_EXCLUSIVE_WRITER_SCOPED(var) do { } while (0)
#define ASSERT_EXCLUSIVE_ACCESS(var) do { } while (0)
#define ASSERT_EXCLUSIVE_ACCESS_SCOPED(var) do { } while (0)
#define ASSERT_EXCLUSIVE_BITS(var, mask) do { } while (0)

#endif /* _LINUX_KCSAN_CHECKS_H */
