#
# arch-arm.mk
# Peter Jones, 2018-11-09 15:28
#
ifeq ($(ARCH),aa64)

ARCH_CCLDFLAGS += -Wl,--defsym=EFI_SUBSYSTEM=$(SUBSYSTEM)
ARCH_CFLAGS ?= -mstrict-align \
	       -DMDE_CPU_AARCH64 \
	       -DPAGE_SIZE=4096 -DPAGE_SHIFT=12
ARCH_LDFLAGS += --defsym=EFI_SUBSYSTEM=$(SUBSYSTEM)
ARCH_SUFFIX ?= aa64
ARCH_SUFFIX_UPPER ?= AA64
FORMAT := -O binary
LDARCH ?= aarch64
LIBDIR ?= $(prefix)/lib64
SUBSYSTEM := 0xa
TIMESTAMP_LOCATION := 72

endif
#
# vim:ft=make
