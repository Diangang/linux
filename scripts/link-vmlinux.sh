#!/bin/sh
# SPDX-License-Identifier: GPL-2.0
#
# link vmlinux
#
# vmlinux is linked from the objects in vmlinux.a and $(KBUILD_VMLINUX_LIBS).
# vmlinux.a contains objects that are linked unconditionally.
# $(KBUILD_VMLINUX_LIBS) are archives which are linked conditionally
# (not within --whole-archive), and do not require symbol indexes added.
#
# vmlinux
#   ^
#   |
#   +--< vmlinux.a
#   |
#   +--< $(KBUILD_VMLINUX_LIBS)
#   |    +--< lib/lib.a + more
#
# vmlinux version (uname -v) cannot be updated during normal
# descending-into-subdirs phase since we do not yet know if we need to
# update vmlinux.
# Therefore this step is delayed until just before final link of vmlinux.
#
# System.map is generated to document addresses of all kernel symbols

# Error out on error
set -e

LD="$1"
KBUILD_LDFLAGS="$2"
LDFLAGS_vmlinux="$3"
VMLINUX="$4"

is_enabled() {
	grep -q "^$1=y" include/config/auto.conf
}

# Nice output in kbuild format
# Will be supressed by "make -s"
info()
{
	printf "  %-7s %s\n" "${1}" "${2}"
}

# Link of vmlinux
# ${1} - output file
vmlinux_link()
{
	local output=${1}
	local objs
	local libs
	local ld
	local ldflags
	local ldlibs

	info LD ${output}

	# skip output file argument
	shift

	if is_enabled CONFIG_X86_KERNEL_IBT; then
		# Use vmlinux.o instead of performing the slow LTO link again.
		objs=vmlinux.o
		libs=
	else
		objs=vmlinux.a
		libs="${KBUILD_VMLINUX_LIBS}"
	fi

	objs="${objs} .vmlinux.export.o"
	objs="${objs} init/version-timestamp.o"

	if [ "${SRCARCH}" = "um" ]; then
		wl=-Wl,
		ld="${CC}"
		ldflags="${CFLAGS_vmlinux}"
		ldlibs="-lutil -lrt -lpthread"
	else
		wl=
		ld="${LD}"
		ldflags="${KBUILD_LDFLAGS} ${LDFLAGS_vmlinux}"
		ldlibs=
	fi

	ldflags="${ldflags} ${wl}--script=${objtree}/${KBUILD_LDS}"

	${ld} ${ldflags} -o ${output}					\
		${wl}--whole-archive ${objs} ${wl}--no-whole-archive	\
		${wl}--start-group ${libs} ${wl}--end-group		\
		${arch_vmlinux_o} ${ldlibs}
}

# Create map file with all symbols from ${1}
# See mksymap for additional details
mksysmap()
{
	info NM ${2}
	${NM} -n "${1}" | sed -f "${srctree}/scripts/mksysmap" > "${2}"
}

sorttable()
{
	${NM} -S ${1} > .tmp_vmlinux.nm-sort
	${objtree}/scripts/sorttable -s .tmp_vmlinux.nm-sort ${1}
}

cleanup()
{
	rm -f .btf.*
	rm -f .tmp_vmlinux.nm-sort
	rm -f System.map
	rm -f vmlinux
	rm -f vmlinux.map
}

# Use "make V=1" to debug this script
case "${KBUILD_VERBOSE}" in
*1*)
	set -x
	;;
esac

if [ "$1" = "clean" ]; then
	cleanup
	exit 0
fi

${MAKE} -f "${srctree}/scripts/Makefile.build" obj=init init/version-timestamp.o

arch_vmlinux_o=

vmlinux_link "${VMLINUX}"

mksysmap "${VMLINUX}" System.map

if is_enabled CONFIG_BUILDTIME_TABLE_SORT; then
	info SORTTAB "${VMLINUX}"
	if ! sorttable "${VMLINUX}"; then
		echo >&2 Failed to sort kernel tables
		exit 1
	fi
fi

# For fixdep
echo "${VMLINUX}: $0" > ".${VMLINUX}.d"
