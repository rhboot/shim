ARCH		?= $(shell $(CC) -dumpmachine | cut -f1 -d- | sed \
			-e s,aarch64,aa64, \
			-e 's,arm.*,arm,' \
			-e s,i[3456789]86,ia32, \
			-e s,x86_64,x64, \
			)
include $(TOPDIR)/include/arch-$(ARCH).mk

COMPILER	?= gcc
CC		= $(CROSS_COMPILE)$(COMPILER)
DESTDIR		?=
LD		= $(CROSS_COMPILE)ld
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
DATATARGETDIR	?= $(datadir)/$(PKGNAME)/$(VERSION)$(DASHRELEASE)/$(ARCH)/
DEBUGINFO	?= $(prefix)/lib/debug/
DEBUGSOURCE	?= $(prefix)/src/debug/
OSLABEL		?= $(EFIDIR)
DEFAULT_LOADER	?= \\\\grub$(ARCH).efi
DASHJ		?= -j$(shell echo $$(($$(grep -c "^model name" /proc/cpuinfo) + 1)))
OBJCOPY_GTE224	= $(shell expr `$(OBJCOPY) --version |grep ^"GNU objcopy" | sed 's/^.*\((.*)\|version\) //g' | cut -f1-2 -d.` \>= 2.24)

SUBDIRS		= $(TOPDIR)/Cryptlib $(TOPDIR)/lib

EFI_INCLUDE	?= /usr/include/efi
EFI_INCLUDES	= -nostdinc -I$(TOPDIR)/Cryptlib -I$(TOPDIR)/Cryptlib/Include \
		  -I$(EFI_INCLUDE) -I$(EFI_INCLUDE)/$(ARCH) \
		  -I$(EFI_INCLUDE)/protocol \
		  -I$(TOPDIR)/include -iquote $(TOPDIR) -iquote $(shell pwd)

EFI_CRT_OBJS 	= $(EFI_PATH)/crt0-efi-$(ARCH).o
EFI_LDS		= $(TOPDIR)/include/elf_$(ARCH)_efi.lds

COMMIT_ID ?= $(shell if [ -e .git ] ; then git log -1 --pretty=format:%H ; elif [ -f commit ]; then cat commit ; else echo master; fi)

CFLAGS		= -ggdb -O0 -fno-stack-protector -fno-strict-aliasing -fpic \
		  -fshort-wchar -Wall -Wsign-compare -Werror -fno-builtin \
		  -Werror=sign-compare -ffreestanding -std=gnu89 \
		  -I$(shell $(CC) $(ARCH_CFLAGS) -print-file-name=include) \
		  "-DDEFAULT_LOADER=L\"$(DEFAULT_LOADER)\"" \
		  "-DDEFAULT_LOADER_CHAR=\"$(DEFAULT_LOADER)\"" \
		  $(EFI_INCLUDES) $(ARCH_CFLAGS)

ifneq ($(origin OVERRIDE_SECURITY_POLICY), undefined)
	CFLAGS	+= -DOVERRIDE_SECURITY_POLICY
endif

ifneq ($(origin ENABLE_HTTPBOOT), undefined)
	CFLAGS	+= -DENABLE_HTTPBOOT
endif

ifneq ($(origin REQUIRE_TPM), undefined)
	CFLAGS  += -DREQUIRE_TPM
endif

LIB_GCC		= $(shell $(CC) $(ARCH_CFLAGS) -print-libgcc-file-name)
EFI_LIBS	= -lefi -lgnuefi --start-group Cryptlib/libcryptlib.a Cryptlib/OpenSSL/libopenssl.a --end-group $(LIB_GCC)
FORMAT		?= --target efi-app-$(LDARCH)
EFI_PATH	?= $(LIBDIR)/gnuefi

MMSTEM		?= mm$(ARCH)
MMNAME		= $(MMSTEM).efi
MMSONAME	= $(MMSTEM).so
FBSTEM		?= fb$(ARCH)
FBNAME		= $(FBSTEM).efi
FBSONAME	= $(FBSTEM).so
SHIMSTEM	?= shim$(ARCH)
SHIMNAME	= $(SHIMSTEM).efi
SHIMSONAME	= $(SHIMSTEM).so
SHIMHASHNAME	= $(SHIMSTEM).hash
BOOTEFINAME	?= BOOT$(ARCH_UPPER).EFI
BOOTCSVNAME	?= BOOT$(ARCH_UPPER).CSV

CFLAGS += "-DEFI_ARCH=L\"$(ARCH)\"" "-DDEBUGDIR=L\"/usr/lib/debug/usr/share/shim/$(ARCH)-$(VERSION)$(DASHRELEASE)/\""

ifneq ($(origin VENDOR_CERT_FILE), undefined)
	CFLAGS += -DVENDOR_CERT_FILE=\"$(VENDOR_CERT_FILE)\"
endif
ifneq ($(origin VENDOR_DBX_FILE), undefined)
	CFLAGS += -DVENDOR_DBX_FILE=\"$(VENDOR_DBX_FILE)\"
endif

LDFLAGS		= --hash-style=sysv -nostdlib -znocombreloc -T $(EFI_LDS) -shared -Bsymbolic -L$(EFI_PATH) -L$(LIBDIR) -LCryptlib -LCryptlib/OpenSSL $(EFI_CRT_OBJS) --build-id=sha1 $(ARCH_LDFLAGS) --no-undefined

define get-config
$(shell git config --local --get "shim.$(1)")
endef

.EXPORT_ALL_VARIABLES:
unexport KEYS
unexport FALLBACK_OBJS FALLBACK_SRCS
unexport MOK_OBJS MOK_SOURCES
unexport OBJS ORIG_FALLBACK_SRCS ORIG_SOURCES ORIG_MOK_SOURCES
unexport SOURCES SUBDIRS
unexport TARGET TARGETS
unexport VPATH
