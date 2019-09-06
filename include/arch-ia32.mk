#
# arch-ia32.mk
# Peter Jones, 2018-11-09 15:26
#
ifeq ($(EFI_ARCH),ia32)

EFI_ARCH_CPPFLAGS += -DMDE_CPU_IA32

export EFI_ARCH_CPPFLAGS
endif
#
# vim:ft=make
