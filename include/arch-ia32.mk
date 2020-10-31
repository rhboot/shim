#
# arch-ia32.mk
# Ruslan Nikolaev
#
ifeq ($(EFI_ARCH),ia32)

CLANG_BUGS = $(if $(findstring gcc,$(CC)),-maccumulate-outgoing-args,)

EFI_ARCH_CFLAGS ?= -mno-mmx -mno-sse -mno-red-zone -nostdinc \
	$(CLANG_BUGS) -mno-stack-arg-probe -m32 \
	-DPAGE_SIZE=4096 -DPAGE_SHIFT=12 \
	-mregparm=3 -mrtd -march=i586 -target i586-windows-gnu

EFI_ARCH_LDFLAGS ?= /machine:x86
EFI_ARCH_HEADERS ?= Ia32
EFI_ARCH_SUFFIX ?= ia32
EFI_ARCH_UPPER ?= IA32

ifneq ($(DEBUG),yes)
	EFI_DEBUG_CFLAGS += -momit-leaf-frame-pointer
endif

endif
#
# vim:ft=make
