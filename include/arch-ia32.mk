#
# arch-ia32.mk
# Peter Jones, 2018-11-09 15:26
#
ifeq ($(ARCH),ia32)

ARCH_CFLAGS ?= -mno-mmx -mno-sse -mno-red-zone -nostdinc \
	       $(CLANG_BUGS) -m32 \
	       -DMDE_CPU_IA32 \
	       -DPAGE_SIZE=4096 -DPAGE_SHIFT=12
ARCH_LDFLAGS ?=
ARCH_SUFFIX ?= ia32
ARCH_SUFFIX_UPPER ?= IA32
CLANG_BUGS = $(if $(findstring gcc,$(CC)),-maccumulate-outgoing-args,)
LDARCH ?= ia32
LIBDIR ?= $(prefix)/lib
TIMESTAMP_LOCATION := 136

endif
#
# vim:ft=make
