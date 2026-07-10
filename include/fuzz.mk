# SPDX-License-Identifier: BSD-2-Clause-Patent
#
# fuzz.mk - makefile to fuzz local test programs
#

.SUFFIXES:

include $(TOPDIR)/Make.defaults

CC = clang
DEBUG_PRINTS ?= 0
OPTIMIZATIONS ?= -Og -ggdb
FUZZ_ARGS ?=
MAX_FUZZ_TIME ?=
CFLAGS = $(OPTIMIZATIONS) -std=gnu11 \
	 -isystem $(TOPDIR)/include/system \
	 $(EFI_INCLUDES) \
	 -Iinclude -iquote . \
	 -isystem /usr/include \
	 -isystem $(shell $(CC) $(ARCH_CFLAGS) -print-file-name=include) \
	 $(ARCH_CFLAGS) \
	 -fsanitize=fuzzer,address \
	 -fshort-wchar \
	 -fno-builtin \
	 -rdynamic \
	 -fno-inline \
	 $(if $(findstring gcc,$(CC)),-fno-eliminate-unused-debug-types) \
	 $(if $(findstring gcc,$(CC)),-fno-eliminate-unused-debug-symbols) \
	 -gpubnames \
	 -grecord-gcc-switches \
	 $(if $(findstring clang,$(CC)),-Wno-unknown-warning-option) \
	 $(DEFAULT_WARNFLAGS) \
	 -Wsign-compare \
	 -Wno-deprecated-declarations \
	 $(if $(findstring gcc,$(CC)),-Wno-unused-but-set-variable) \
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
	 -DSHIM_ENABLE_LIBFUZZER \
	 "-DDEFAULT_DEBUG_PRINT_STATE=$(DEBUG_PRINTS)"

# On some systems (e.g. Arch Linux), limits.h is in the "include-fixed" instead
# of the "include" directory
INCLUDE_FIXED = $(shell $(CC) $(ARCH_CFLAGS) -print-file-name=include-fixed)
ifneq ($(strip $(INCLUDE_FIXED)),include-fixed)
CFLAGS += -isystem $(INCLUDE_FIXED)
endif

# And on Debian also check the multi-arch include path
MULTIARCH_ISYSTEM = $(if $(findstring gcc,$(CC)),$(shell $(CC) $(ARCH_CFLAGS) -print-multiarch))
ifneq ($(strip $(MULTIARCH_ISYSTEM)),)
CFLAGS += -isystem /usr/include/$(MULTIARCH_ISYSTEM)
endif

libefi-test.a :
	$(MAKE) -C gnu-efi \
		COMPILER="$(COMPILER)" \
		CC="$(CC)" \
		ARCH=$(ARCH_GNUEFI) \
		TOPDIR=$(TOPDIR)/gnu-efi \
		-f $(TOPDIR)/gnu-efi/Makefile \
		clean lib
	mv gnu-efi/$(ARCH)/lib/libefi.a $@
	$(MAKE) -C gnu-efi \
		COMPILER="$(COMPILER)" \
		ARCH=$(ARCH_GNUEFI) \
		TOPDIR=$(TOPDIR)/gnu-efi \
		-f $(TOPDIR)/gnu-efi/Makefile \
		clean

fuzz-tftpboot_FILES = lib/string.c
fuzz-tftpboot :: private FUZZ_ARGS=-dict=$(TOPDIR)/data/$@-dict.txt

fuzz-pe-relocate_FILES = globals.c

generated_sbat_var_defs.h :
	$(MAKE) generated_sbat_var_defs.h

fuzz-sbat_FILES = csv.c lib/variables.c lib/guid.c sbat_var.S mock-variables.c
fuzz-sbat :: | generated_sbat_var_defs.h
fuzz-sbat :: CFLAGS+=-DHAVE_GET_VARIABLE -DHAVE_GET_VARIABLE_ATTR -DHAVE_SHIM_LOCK_GUID

fuzzers := $(patsubst %.c,%,$(wildcard fuzz-*.c))

$(fuzzers) :: fuzz-% : | libefi-test.a

ifneq ($(strip $(MAX_FUZZ_TIME)),)
MAX_TOTAL_TIME=-max_total_time=$(MAX_FUZZ_TIME)
else
MAX_TOTAL_TIME=
endif

$(fuzzers) :: fuzz-% : test.c fuzz-%.c $(fuzz-%_FILES)
	$(CC) $(CFLAGS) -o $@ $(sort $^ $(wildcard $*.c) $(fuzz-$*_FILES)) libefi-test.a -lefivar
	mkdir -p $@-corpus
	cd $@-corpus ; ../$@ \
		-jobs=24 \
		-max_len=4096 \
		$(MAX_TOTAL_TIME) \
		$(FUZZ_ARGS) \
		../$@-corpus

fuzz : $(fuzzers)
	$(MAKE) -f include/fuzz.mk fuzz-clean

fuzz-clean :
	@rm -vf random.bin libefi-test.a
	@rm -vf vgcore.* fuzz*.log

clean : fuzz-clean

all : fuzz-clean fuzz

.PHONY: $(fuzzers) all fuzz clean
.SECONDARY: random.bin

# vim:ft=make
