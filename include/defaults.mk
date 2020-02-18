.SUFFIXES:

COMPILER	?= gcc
CC		= $(CROSS_COMPILE)$(COMPILER)

ARCH		?= $(shell $(CC) -dumpmachine | cut -f1 -d- | sed \
			-e s,aarch64,aa64, \
			-e 's,arm.*,arm,' \
			-e s,i[3456789]86,ia32, \
			-e s,x86_64,x64, \
			)
COMPILER	?= gcc
CC		= $(CROSS_COMPILE)$(COMPILER)
BUILDDIR	?= $(TOPDIR)
DESTDIR		?=
LD		= $(CROSS_COMPILE)ld
AR		= $(CROSS_COMPILE)gcc-ar
OBJCOPY		= $(CROSS_COMPILE)objcopy
OPENSSL		?= openssl
HEXDUMP		?= hexdump
INSTALL		?= install
PK12UTIL	?= pk12util
CERTUTIL	?= certutil
PESIGN		?= pesign
SBSIGN		?= sbsign
prefix		?= /usr
prefix		:= $(abspath $(prefix))
datadir		?= $(prefix)/share/
PKGNAME		?= shim
ESPROOTDIR	?= boot/efi/
EFIBOOTDIR	?= $(ESPROOTDIR)EFI/BOOT/
TARGETDIR	?= $(ESPROOTDIR)EFI/$(EFIDIR)/
DATATARGETDIR	?= $(datadir)/$(PKGNAME)/$(VERSION)$(DASHRELEASE)/$(EFI_ARCH)/
DEBUGINFO	?= $(prefix)/lib/debug/
DEBUGSOURCE	?= $(prefix)/src/debug/
OSLABEL		?= $(EFIDIR)
DEFAULT_LOADER	?= \\\\grub$(EFI_ARCH_SUFFIX).efi
DASHJ		?= -j$(shell echo $$(($$(grep -c "^model name" /proc/cpuinfo) + 1)))
OPTIMIZATIONS	?= -Os

OPENSSLDIR	= $(TOPDIR)/Cryptlib/OpenSSL

CC_LTO_PLUGIN = -flto=jobserver -fuse-linker-plugin -ffat-lto-objects

EFI_INCLUDES	+= \
		   -I$(TOPDIR)/include \
		   -iquote $(TOPDIR) \
		   -iquote $(TOPDIR)/src \
		   -iquote $(OPENSSLDIR) \
		   -iquote $(OPENSSLDIR)/crypto/include \
		   -iquote $(BUILDDIR)/src \
		   -iquote $(TOPDIR)/include

COMMIT_ID ?= $(shell if [ -e .git ] ; then git log -1 --pretty=format:%H ; elif [ -f commit ]; then cat commit ; else echo master; fi)

EFI_LDSCRIPT = $(TOPDIR)/include/elf_$(EFI_ARCH)_efi.lds
OPENSSL_CFLAGS ?= -Wno-error=pointer-sign
OPENSSL_CPPFLAGS ?=

EFI_CPPFLAGS += $(OPENSSL_CPPFLAGS) \
		-Werror=sign-compare -std=gnu11
EFI_CFLAGS += -ggdb3 $(OPTIMIZATIONS) -Wall -Wsign-compare -Werror $(CC_LTO_PLUGIN) \
	      $(OPENSSL_CFLAGS)

define get-config
$(shell git config --local --get "shim.$(1)")
endef

define object-template
$(subst $(TOPDIR),$(BUILDDIR),$(patsubst %.c,%.efi.o,$(2))): $(2)
	$$(EFI_CC) $(3) $$(EFI_CFLAGS) -c -o $$@ $$<
endef

%.efi : $(BUILDDIR)/%.efi

src/%.o : $(BUILDDIR)/src/%.o

%.crt : $(BUILDDIR)/certdb/%.crt
%.cer : $(BUILDDIR)/certdb/%.cer

include efi.mk
include $(TOPDIR)/include/arch-$(EFI_ARCH).mk

ifneq ($(origin ENABLE_SBSIGN),undefined)
%.efi.signed: %.efi shim.key shim.crt
	@$(SBSIGN) \
		--key $(BUILDDIR)/certdb/shim.key \
		--cert $(BUILDDIR)/certdb/shim.crt \
		--output $(BUILDDIR)/$@ $(BUILDDIR)/$^
else
.ONESHELL: $(MMNAME).signed $(FBNAME).signed
%.efi.signed: %.efi certdb/secmod.db
	@cd $(BUILDDIR)
	$(PESIGN) -n certdb -i $< -c "shim" -s -o $@ -f
endif

.ONESHELL: %.hash
%.hash : | $(BUILDDIR)
%.hash : %.efi
	@cd $(BUILDDIR)
	$(PESIGN) -i $< -P -h > $@

%.efi %.efi.signed %.efi.debug %.efi.so : | $(BUILDDIR)

.EXPORT_ALL_VARIABLES:
unexport KEYS
unexport FALLBACK_OBJS FALLBACK_SRCS
unexport MOK_OBJS MOK_SOURCES
unexport OBJS
unexport SOURCES SUBDIRS
unexport TARGET TARGETS
