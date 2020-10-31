.SUFFIXES:

COMPILER	?= clang
CC		= $(CROSS_COMPILE)$(COMPILER)

ARCH		?= $(shell $(CC) -dumpmachine | cut -f1 -d- | sed \
			-e s,aarch64,aa64, \
			-e 's,arm.*,arm,' \
			-e s,i[3456789]86,ia32, \
			-e s,x86_64,x64, \
			)
COMPILER	?= clang
LINKER		?= ld.lld
CC		= $(CROSS_COMPILE)$(COMPILER)
BUILDDIR	?= $(TOPDIR)
DESTDIR		?=
LD		= $(CROSS_COMPILE)$(LINKER)
AR		= $(CROSS_COMPILE)llvm-ar
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
DEBUGINFO	?= $(prefix)/lib/debug/
DEBUGSOURCE	?= $(prefix)/src/debug/
DEBUG		?= no
OSLABEL		?= $(EFIDIR)
DASHJ		?= -j$(shell echo $$(($$(grep -c "^model name" /proc/cpuinfo) + 1)))

ifeq ($(DEBUG),yes)
EFI_DEBUG_CFLAGS	= -g -O0
EFI_DEBUG_LDFLAGS	= /debug
else
EFI_DEBUG_CFLAGS	= -Os
EFI_DEBUG_LDFLAGS	=
endif

include $(TOPDIR)/include/arch-$(EFI_ARCH).mk

OPENSSLDIR	= $(TOPDIR)/Cryptlib/OpenSSL

EFI_INCLUDES	+= \
		   -I$(TOPDIR)/include \
		   -I$(TOPDIR)/edk2/MdePkg/Include \
		   -I$(TOPDIR)/edk2/MdePkg/Include/$(EFI_ARCH_HEADERS) \
		   -iquote $(TOPDIR) \
		   -iquote $(TOPDIR)/src \
		   -iquote $(OPENSSLDIR) \
		   -iquote $(OPENSSLDIR)/crypto/include \
		   -iquote $(BUILDDIR)/src \
		   -iquote $(TOPDIR)/include

COMMIT_ID ?= $(shell if [ -e .git ] ; then git log -1 --pretty=format:%H ; elif [ -f commit ]; then cat commit ; else echo master; fi)

OPENSSL_CFLAGS ?= -Wno-error=pointer-sign
OPENSSL_CPPFLAGS ?=

EFI_CFLAGS = -Wall -Wsign-compare -Werror -Werror=sign-compare -std=gnu11 \
		-ffreestanding -fshort-wchar -fno-builtin -fno-stack-protector -flto \
		-fno-strict-aliasing -UWIN32 -U_WIN32 -U__CYGWIN_ -ffunction-sections \
		-fdata-sections -integrated-as $(OPENSSL_CFLAGS) $(EFI_ARCH_CFLAGS) \
		$(EFI_DEBUG_CFLAGS)

define get-config
$(shell git config --local --get "shim.$(1)")
endef

define object-template
$(subst $(TOPDIR),$(BUILDDIR),$(patsubst %.S,%.efi.o,$(patsubst %.c,%.efi.o,$(2)))): $(2)
	$$(CC) $(3) $$(EFI_CFLAGS) $$(OPENSSL_CPPFLAGS) $$(EFI_INCLUDES) $$(OPENSSL_INCLUDES) -c -o $$@ $$<
endef

%.efi : $(BUILDDIR)/%.efi

src/%.o : $(BUILDDIR)/src/%.o

%.crt : $(BUILDDIR)/certdb/%.crt
%.cer : $(BUILDDIR)/certdb/%.cer

EFI_LDFLAGS = $(EFI_DEBUG_LDFLAGS) /dll /entry:efi_main /nodefaultlib $(EFI_ARCH_LDFLAGS)

%.efi: %.efi.dll fwimage/fwimage
	./fwimage/fwimage app $< $@
	@chmod 755 $@

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

%.efi %.efi.signed %.efi.dll : | $(BUILDDIR)

.EXPORT_ALL_VARIABLES:
unexport KEYS
unexport FALLBACK_OBJS FALLBACK_SRCS
unexport MOK_OBJS MOK_SOURCES
unexport OBJS
unexport SOURCES SUBDIRS
unexport TARGET TARGETS
