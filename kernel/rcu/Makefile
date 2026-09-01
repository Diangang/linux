# SPDX-License-Identifier: GPL-2.0
# Any varying coverage in these files is non-deterministic
# and is generally not a function of system call inputs.

obj-y += update.o sync.o
obj-$(CONFIG_TREE_SRCU) += srcutree.o
obj-$(CONFIG_TREE_RCU) += tree.o
obj-$(CONFIG_RCU_NEED_SEGCBLIST) += rcu_segcblist.o
