# SPDX-License-Identifier: GPL-2.0
# Any varying coverage in these files is non-deterministic
# and is generally not a function of system call inputs.
KCOV_INSTRUMENT		:= n

CONTEXT_ANALYSIS_mutex.o := y
CONTEXT_ANALYSIS_rtmutex_api.o := y
CONTEXT_ANALYSIS_ww_rt_mutex.o := y
CONTEXT_ANALYSIS_rwsem.o := y

obj-y += mutex.o semaphore.o rwsem.o percpu-rwsem.o

# Avoid recursion lockdep -> sanitizer -> ... -> lockdep & improve performance.
KASAN_SANITIZE_lockdep.o := n
KCSAN_SANITIZE_lockdep.o := n

obj-$(CONFIG_LOCKDEP) += lockdep.o
ifeq ($(CONFIG_PROC_FS),y)
obj-$(CONFIG_LOCKDEP) += lockdep_proc.o
endif
obj-$(CONFIG_SMP) += spinlock.o
obj-$(CONFIG_LOCK_SPIN_ON_OWNER) += osq_lock.o
obj-$(CONFIG_QUEUED_SPINLOCKS) += qspinlock.o
obj-$(CONFIG_RT_MUTEXES) += rtmutex_api.o
obj-$(CONFIG_PREEMPT_RT) += spinlock_rt.o ww_rt_mutex.o
obj-$(CONFIG_QUEUED_RWLOCKS) += qrwlock.o
obj-$(CONFIG_LOCK_EVENT_COUNTS) += lock_events.o
