#
# arch-x64.mk
# Peter Jones, 2018-11-09 15:24
#
ifeq ($(ARCH),x64)

ARCH_CCLDFLAGS ?= -ffreestanding
ARCH_CFLAGS ?= -mno-mmx -mno-sse -mno-red-zone -nostdinc \
	       $(CLANG_BUGS) -m64 \
	       -DEFI_FUNCTION_WRAPPER -DGNU_EFI_USE_MS_ABI \
	       -DNO_BUILTIN_VA_FUNCS -DMDE_CPU_X64 \
	       -DPAGE_SIZE=4096 -DPAGE_SHIFT=12
ARCH_LDFLAGS ?=
ARCH_SUFFIX ?= x64
ARCH_SUFFIX_UPPER ?= X64
CLANG_BUGS = $(if $(findstring gcc,$(CC)),-maccumulate-outgoing-args,)
LDARCH ?= x86_64
LIBDIR ?= $(prefix)/lib64
TIMESTAMP_LOCATION := 136

endif
#
# vim:ft=make
