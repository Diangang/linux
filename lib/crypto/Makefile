# SPDX-License-Identifier: GPL-2.0

# blake2s is used by the /dev/random driver which is always builtin
obj-y += blake2s.o
ifeq ($(CONFIG_CRYPTO_LIB_BLAKE2S_ARCH),y)
CFLAGS_blake2s.o += -I$(src)/$(SRCARCH)
obj-$(CONFIG_X86) += x86/blake2s-core.o
endif

# chacha20_block() is used by the /dev/random driver which is always builtin
obj-y += chacha-block-generic.o
