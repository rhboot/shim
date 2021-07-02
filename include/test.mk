# SPDX-License-Identifier: BSD-2-Clause-Patent
#
# test.mk - makefile to make local test programs
#

.SUFFIXES:

include Make.defaults

CC = gcc
VALGRIND ?=
DEBUG_PRINTS ?= 0
OPTIMIZATIONS=-O2 -ggdb
CFLAGS = $(OPTIMIZATIONS) -std=gnu11 \
	 -isystem $(TOPDIR)/include/system \
	 $(EFI_INCLUDES) \
	 -Iinclude -iquote . \
	 -isystem /usr/include \
	 -isystem $(shell $(CC) $(ARCH_CFLAGS) -print-file-name=include) \
	 $(ARCH_CFLAGS) \
	 -fshort-wchar \
	 -flto \
	 -fno-builtin \
	 -rdynamic \
	 -fno-inline \
	 -fno-eliminate-unused-debug-types \
	 -fno-eliminate-unused-debug-symbols \
	 -gpubnames \
	 -grecord-gcc-switches \
	 $(DEFAULT_WARNFLAGS) \
	 -Wsign-compare \
	 -Wno-deprecated-declarations \
	 -Wno-unused-but-set-variable \
	 -Wno-unused-variable \
	 -Wno-pointer-sign \
	 $(DEFAULT_WERRFLAGS) \
	 -Werror=nonnull \
	 $(shell $(CC) -Werror=nonnull-compare -E -x c /dev/null >/dev/null 2>&1 && echo -Werror=nonnull-compare) \
	 $(ARCH_DEFINES) \
	 -DEFI_FUNCTION_WRAPPER \
	 -DGNU_EFI_USE_MS_ABI -DPAGE_SIZE=4096 \
	 -DSHIM_UNIT_TEST \
	 "-DDEFAULT_DEBUG_PRINT_STATE=$(DEBUG_PRINTS)"

libefi-test.a :
	$(MAKE) -C gnu-efi ARCH=$(ARCH_GNUEFI) TOPDIR=$(TOPDIR)/gnu-efi \
		-f $(TOPDIR)/gnu-efi/Makefile \
		clean lib
	mv gnu-efi/$(ARCH)/lib/libefi.a $@
	$(MAKE) -C gnu-efi ARCH=$(ARCH_GNUEFI) TOPDIR=$(TOPDIR)/gnu-efi \
		-f $(TOPDIR)/gnu-efi/Makefile \
		clean

test-random.h:
	dd if=/dev/urandom bs=512 count=17 of=random.bin
	xxd -i random.bin test-random.h

$(wildcard test-*.c) :: %.c : test-random.h
$(patsubst %.c,%,$(wildcard test-*.c)) :: | test-random.h
$(patsubst %.c,%.o,$(wildcard test-*.c)) : | test-random.h

test-load-options_FILES = lib/guid.c \
			  libefi-test.a \
			  -lefivar
test-load-options :: libefi-test.a
test-load-options : CFLAGS+=-DHAVE_SHIM_LOCK_GUID

test-sbat_FILES = csv.c lib/variables.c lib/guid.c
test-sbat :: CFLAGS+=-DHAVE_GET_VARIABLE -DHAVE_GET_VARIABLE_ATTR -DHAVE_SHIM_LOCK_GUID

test-str_FILES = lib/string.c

tests := $(patsubst %.c,%,$(wildcard test-*.c))

$(tests) :: test-% : | libefi-test.a

$(tests) :: test-% : test.c test-%.c $(test-%_FILES)
	$(CC) $(CFLAGS) -o $@ $^ $(wildcard $*.c) $(test-$*_FILES) libefi-test.a
	$(VALGRIND) ./$@

test : $(tests)

clean :
	@rm -vf test-random.h random.bin libefi-test.a

all : clean test

.PHONY: $(tests) all test clean
.SECONDARY: random.bin

# vim:ft=make
