# SPDX-License-Identifier: GPL-2.0

# blake2s is used by the /dev/random driver which is always builtin
obj-y += blake2s.o
ifeq ($(CONFIG_CRYPTO_LIB_BLAKE2S_ARCH),y)
CFLAGS_blake2s.o += -I$(src)/$(SRCARCH)
obj-$(CONFIG_X86) += x86/blake2s-core.o
endif

# chacha20_block() is used by the /dev/random driver which is always builtin
obj-y += chacha-block-generic.o

obj-$(CONFIG_CRYPTO_LIB_SHA256) += libsha256.o
libsha256-y := sha256.o
ifeq ($(CONFIG_CRYPTO_LIB_SHA256_ARCH),y)
CFLAGS_sha256.o += -I$(src)/$(SRCARCH)
libsha256-$(CONFIG_X86) += x86/sha256-ssse3-asm.o \
			   x86/sha256-avx-asm.o \
			   x86/sha256-avx2-asm.o \
			   x86/sha256-ni-asm.o
endif
