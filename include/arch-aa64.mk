#
# arch-aa64.mk
# Ruslan Nikolaev
#
ifeq ($(EFI_ARCH),aa64)

EFI_ARCH_CFLAGS ?= -mstrict-align \
	-mno-stack-arg-probe \
	-DPAGE_SIZE=4096 -DPAGE_SHIFT=12 \
	-target aarch64-windows-gnu

EFI_ARCH_LDFLAGS ?= /machine:arm64
EFI_ARCH_HEADERS ?= AArch64
EFI_ARCH_SUFFIX ?= aa64
EFI_ARCH_UPPER ?= AA64

endif
#
# vim:ft=make
