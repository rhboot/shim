#
# arch-arm.mk
# Peter Jones, 2018-11-09 15:28
#
ifeq ($(ARCH),arm)

ARCH_CFLAGS ?= -DMDE_CPU_ARM -DPAGE_SIZE=4096 -DPAGE_SHIFT=12 \
	       -mno-unaligned-access
ARCH_LDFLAGS += --defsym=EFI_SUBSYSTEM=$(SUBSYSTEM)
ARCH_SUFFIX ?= arm
ARCH_SUFFIX_UPPER ?= ARM
FORMAT := -O binary
LDARCH ?= arm
LIBDIR ?= $(prefix)/lib
SUBSYSTEM := 0xa
TIMESTAMP_LOCAITON := 72

endif
#
# vim:ft=make
