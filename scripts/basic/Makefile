# SPDX-License-Identifier: GPL-2.0-only
#
# fixdep: used to generate dependency information during build process

hostprogs-always-y	+= fixdep

# integer-wrap: if the .scl file changes, we need to do a full rebuild.
$(obj)/../../include/generated/integer-wrap.h: $(srctree)/scripts/integer-wrap-ignore.scl FORCE
	$(call if_changed,touch)
