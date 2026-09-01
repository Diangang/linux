# SPDX-License-Identifier: GPL-2.0
#
# Makefile for the linux kernel.
#

obj-y     = fork.o exec_domain.o panic.o \
	    cpu.o exit.o softirq.o resource.o \
	    sysctl.o capability.o ptrace.o user.o \
	    signal.o sys.o umh.o workqueue.o pid.o task_work.o \
	    extable.o params.o \
	    kthread.o sys_ni.o nsproxy.o nstree.o nscommon.o \
	    notifier.o ksysfs.o cred.o reboot.o \
	    async.o range.o smpboot.o ucount.o regset.o ksyms_common.o

obj-$(CONFIG_MULTIUSER) += groups.o

# Prevents flicker of uninteresting __do_softirq()/__local_bh_disable_ip()
# in coverage traces.
# These are called from save_stack_trace() on slub debug path,
# and produce insane amounts of uninteresting coverage.
obj-y += sched/
obj-y += locking/
obj-y += printk/
obj-y += irq/
obj-y += rcu/
obj-y += dma/
obj-y += entry/
obj-$(CONFIG_STACKTRACE) += stacktrace.o
obj-y += time/
obj-$(CONFIG_FUTEX) += futex/
obj-$(CONFIG_GENERIC_ISA_DMA) += dma.o
obj-$(CONFIG_SMP) += smp.o
ifneq ($(CONFIG_SMP),y)
obj-y += up.o
endif
obj-$(CONFIG_SMP) += stop_machine.o
obj-$(CONFIG_SYSCTL) += utsname_sysctl.o
obj-$(CONFIG_IRQ_WORK) += irq_work.o
obj-$(CONFIG_HAVE_STATIC_CALL) += static_call.o
obj-$(CONFIG_HAVE_STATIC_CALL_INLINE) += static_call_inline.o

obj-$(CONFIG_JUMP_LABEL) += jump_label.o
obj-$(CONFIG_CONTEXT_TRACKING) += context_tracking.o

obj-$(CONFIG_HAS_IOMEM) += iomem.o



$(obj)/configs.o: $(obj)/config_data.gz

targets += config_data config_data.gz
$(obj)/config_data.gz: $(obj)/config_data FORCE
	$(call if_changed,gzip)

filechk_cat = cat $<

$(obj)/config_data: $(KCONFIG_CONFIG) FORCE
	$(call filechk,cat)

# kheaders_data.tar.xz
$(obj)/kheaders.o: $(obj)/kheaders_data.tar.xz

quiet_cmd_kheaders_data = GEN     $@
      cmd_kheaders_data = "$<" "$@" "$(obj)/kheaders-srclist" "$(obj)/kheaders-objlist" "$(KBUILD_BUILD_TIMESTAMP)"
      cmd_kheaders_data_dep = cat $(depfile) >> $(dot-target).cmd; rm -f $(depfile)

define rule_kheaders_data
	$(call cmd_and_savecmd,kheaders_data)
	$(call cmd,kheaders_data_dep)
endef

targets += kheaders_data.tar.xz
$(obj)/kheaders_data.tar.xz: $(src)/gen_kheaders.sh $(obj)/kheaders-srclist $(obj)/kheaders-objlist $(obj)/kheaders.md5 FORCE
	$(call if_changed_rule,kheaders_data)

# generated headers in objtree
#
# include/generated/utsversion.h is ignored because it is generated
# after gen_kheaders.sh is executed. (utsversion.h is unneeded for kheaders)
filechk_kheaders_objlist = \
	for d in include "arch/$(SRCARCH)/include"; do \
		find "$${d}/generated" ! -path "include/generated/utsversion.h" -a -name "*.h" -print; \
	done

$(obj)/kheaders-objlist: FORCE
	$(call filechk,kheaders_objlist)

# non-generated headers in srctree
filechk_kheaders_srclist = \
	for d in include "arch/$(SRCARCH)/include"; do \
		find "$(srctree)/$${d}" -path "$(srctree)/$${d}/generated" -prune -o -name "*.h" -print; \
	done

$(obj)/kheaders-srclist: FORCE
	$(call filechk,kheaders_srclist)

# Some files are symlinks. If symlinks are changed, kheaders_data.tar.xz should
# be rebuilt.
filechk_kheaders_md5sum = xargs -r -a $< stat -c %N | md5sum

$(obj)/kheaders.md5: $(obj)/kheaders-srclist FORCE
	$(call filechk,kheaders_md5sum)

clean-files := kheaders.md5 kheaders-srclist kheaders-objlist
