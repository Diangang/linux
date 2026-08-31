# SPDX-License-Identifier: GPL-2.0
#
# 64-bit vDSO images for x86.
#

# The vDSOs built in this directory
vdsos-y				:= 64

# Files to link into the vDSO:
vobjs-y				:= note.o vclock_gettime.o vgetcpu.o
vobjs-y				+= vgetrandom.o vgetrandom-chacha.o

# Compilation flags
flags-y				:= -DBUILD_VDSO64 -m64 -mcmodel=small

# The location of this include matters!
include $(src)/../common/Makefile.include

# Linker options for the vdso
VDSO_LDFLAGS_64  := -m elf_x86_64 -soname linux-vdso.so.1 -z max-page-size=4096

$(obj)/vdso64.so.dbg:	$(vobjs)
