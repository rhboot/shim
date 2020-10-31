#
# arch-x64.mk
# Ruslan Nikolaev
#
ifeq ($(EFI_ARCH),x64)

CLANG_BUGS = $(if $(findstring gcc,$(CC)),-maccumulate-outgoing-args,)

EFI_ARCH_CFLAGS ?= -mno-red-zone -nostdinc \
	$(CLANG_BUGS) -mno-stack-arg-probe -m64 \
	-DPAGE_SIZE=4096 -DPAGE_SHIFT=12 \
	-target x86_64-windows-gnu

EFI_ARCH_LDFLAGS ?= /machine:x64
EFI_ARCH_HEADERS ?= X64
EFI_ARCH_SUFFIX ?= x64
EFI_ARCH_UPPER ?= X64

ifneq ($(DEBUG),yes)
	EFI_DEBUG_CFLAGS += -momit-leaf-frame-pointer
endif

endif
#
# vim:ft=make
