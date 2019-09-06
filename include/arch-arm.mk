#
# arch-arm.mk
# Peter Jones, 2018-11-09 15:28
#
ifeq ($(EFI_ARCH),arm)

EFI_ARCH_CPPFLAGS += -DMDE_CPU_ARM

export EFI_ARCH_CPPFLAGS

endif
#
# vim:ft=make
