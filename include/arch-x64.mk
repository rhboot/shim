#
# arch-x64.mk
# Peter Jones, 2018-11-09 15:24
#
ifeq ($(EFI_ARCH),x64)

EFI_ARCH_CPPFLAGS += -DNO_BUILTIN_VA_FUNCS -DMDE_CPU_X64

export EFI_ARCH_CPPFLAGS
endif
#
# vim:ft=make
