#
# arch-arm.mk
# Ruslan Nikolaev
#
ifeq ($(EFI_ARCH),arm)

EFI_ARCH_CFLAGS ?= -mno-unaligned-access \
	-mno-stack-arg-probe \
	-DPAGE_SIZE=4096 -DPAGE_SHIFT=12 \
	-target arm-windows-gnu

EFI_ARCH_LDFLAGS ?= /machine:arm
EFI_ARCH_HEADERS ?= Arm
EFI_ARCH_SUFFIX ?= arm
EFI_ARCH_UPPER ?= ARM

endif
#
# vim:ft=make
