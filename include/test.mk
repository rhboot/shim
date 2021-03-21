# SPDX-License-Identifier: BSD-2-Clause-Patent
#
# test.mk - makefile to make local test programs
#

.SUFFIXES:

CC = gcc
VALGRIND ?=
DEBUG_PRINTS ?= 0
CFLAGS = -O2 -ggdb -std=gnu11 \
	 -isystem $(TOPDIR)/include/system \
	 $(EFI_INCLUDES) \
	 -Iinclude -iquote . \
	 -fshort-wchar -flto -fno-builtin \
	 -Wall \
	 -Wextra \
	 -Wsign-compare \
	 -Wno-deprecated-declarations \
	 -Wno-pointer-sign \
	 -Wno-unused \
	 -Werror \
	 -Werror=nonnull \
	 $(shell $(CC) -Werror=nonnull-compare -E -x c /dev/null >/dev/null 2>&1 && echo -Werror=nonnull-compare) \
	 $(ARCH_DEFINES) \
	 -DEFI_FUNCTION_WRAPPER \
	 -DGNU_EFI_USE_MS_ABI -DPAGE_SIZE=4096 \
	 -DSHIM_UNIT_TEST \
	 "-DDEFAULT_DEBUG_PRINT_STATE=$(DEBUG_PRINTS)"

$(wildcard test-*.c) :: %.c : test-random.h
$(patsubst %.c,%,$(wildcard test-*.c)) :: | test-random.h
$(patsubst %.c,%.o,$(wildcard test-*.c)) : | test-random.h

test-random.h:
	dd if=/dev/urandom bs=512 count=17 of=random.bin
	xxd -i random.bin test-random.h

test-sbat_FILES = csv.c
test-str_FILES = lib/string.c

tests := $(patsubst %.c,%,$(wildcard test-*.c))

$(tests) :: test-% : test.c test-%.c $(test-%_FILES)
	$(CC) $(CFLAGS) -o $@ $^ $(wildcard $*.c) $(test-$*_FILES)
	$(VALGRIND) ./$@

test : $(tests)

clean :
	@rm -vf test-random.h random.bin

all : clean test

.PHONY: $(tests) all test clean
.SECONDARY: random.bin

# vim:ft=make
