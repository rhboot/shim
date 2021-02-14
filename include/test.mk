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
	 -Werror=nonnull-compare \
	 $(ARCH_DEFINES) \
	 -DEFI_FUNCTION_WRAPPER \
	 -DGNU_EFI_USE_MS_ABI -DPAGE_SIZE=4096 \
	 -DSHIM_UNIT_TEST \
	 "-DDEFAULT_DEBUG_PRINT_STATE=$(DEBUG_PRINTS)"

tests := $(patsubst %.c,%,$(wildcard test-*.c))

$(tests) :: test-% : test.c test-%.c $(test-%_FILES)
	$(CC) $(CFLAGS) -o $@ $^ $(wildcard $*.c) $(test-$*_FILES)
	$(VALGRIND) ./$@

test : $(tests)

all : test

clean :

.PHONY: $(tests) all test clean

# vim:ft=make
