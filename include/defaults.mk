.SUFFIXES:

COMPILER	?= gcc
CC		= $(CROSS_COMPILE)$(COMPILER)

ARCH		?= $(shell $(CC) -dumpmachine | cut -f1 -d- | sed \
			-e s,aarch64,aa64, \
			-e 's,arm.*,arm,' \
			-e s,i[3456789]86,ia32, \
			-e s,x86_64,x64, \
			)
include $(TOPDIR)/include/arch-$(ARCH).mk

COMPILER	?= gcc
CC		= $(CROSS_COMPILE)$(COMPILER)
CCLD		?= $(CC)
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
DATATARGETDIR	?= $(datadir)/$(PKGNAME)/$(VERSION)$(DASHRELEASE)/$(ARCH)/
DEBUGINFO	?= $(prefix)/lib/debug/
DEBUGSOURCE	?= $(prefix)/src/debug/
OSLABEL		?= $(EFIDIR)
DEFAULT_LOADER	?= \\\\grub$(ARCH).efi
DASHJ		?= -j$(shell echo $$(($$(grep -c "^model name" /proc/cpuinfo) + 1)))
OBJCOPY_GTE224	= $(shell expr `$(OBJCOPY) --version |grep ^"GNU objcopy" | sed 's/^.*\((.*)\|version\) //g' | cut -f1-2 -d.` \>= 2.24)
OPTIMIZATIONS	?= -Os

SUBDIRS		= $(TOPDIR)/Cryptlib $(TOPDIR)/lib
OPENSSLDIR	= $(TOPDIR)/Cryptlib/OpenSSL

CC_LTO_PLUGIN = -flto=$(shell grep -c '^processor\s\+:' /proc/cpuinfo) -fuse-linker-plugin -ffat-lto-objects

EFI_INCLUDE	?= /usr/include/efi
EFI_INCLUDES	= \
		  -I$(EFI_INCLUDE) \
		  -I$(EFI_INCLUDE)/$(ARCH) \
		  -I$(EFI_INCLUDE)/protocol

EFI_CRT_OBJS 	= $(EFI_PATH)/crt0-efi-$(ARCH).o
EFI_LDS		= $(TOPDIR)/include/elf_$(ARCH)_efi.lds

COMMIT_ID ?= $(shell if [ -e .git ] ; then git log -1 --pretty=format:%H ; elif [ -f commit ]; then cat commit ; else echo master; fi)

OPENSSL_DEFINES = -D_CRT_SECURE_NO_DEPRECATE \
		 -D_CRT_NONSTDC_NO_DEPRECATE \
		 -DL_ENDIAN \
		 -DOPENSSL_SMALL_FOOTPRINT \
		 -DPEDANTIC

CFLAGS = -ggdb $(OPTIMZATIONS) \
	 -ffreestanding \
	 -fno-builtin \
	 -fno-stack-protector \
	 -fno-strict-aliasing \
	 -fpic \
	 -fshort-wchar \
	 -Wall \
	 -Wsign-compare \
	 -Werror \
	 -Werror=sign-compare \
	 -std=gnu89 \
	 $(ARCH_CFLAGS) \
	 -I$(shell $(CC) $(ARCH_CFLAGS) -print-file-name=include) \
	 $(EFI_INCLUDES) \
	 $(OPENSSL_DEFINES) \
	 $(CC_LTO_PLUGIN)

LIB_GCC		= $(shell $(CC) $(ARCH_CFLAGS) -print-libgcc-file-name)
EFI_LIBS	= -lefi -lgnuefi
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
CCLDFLAGS	= -Wl,--hash-style=sysv \
		  -Wl,-nostdlib,-znocombreloc,--no-undefined \
		  -Wl,-T,$(EFI_LDS) \
		  -Wl,-shared -Wl,-Bsymbolic \
		  -Wl,-L$(EFI_PATH),-L$(LIBDIR) \
		  -Wl,-LCryptlib,-LCryptlib/OpenSSL \
		  -Wl,--build-id=sha1 \
		  -nostdlib \
		  $(EFI_CRT_OBJS) \
		  $(ARCH_CCLDFLAGS)

define get-config
$(shell if [ -e .git ] ; then git config --local --get "shim.$(1)"; fi)
endef

.EXPORT_ALL_VARIABLES:
unexport KEYS
unexport FALLBACK_OBJS FALLBACK_SRCS
unexport MOK_OBJS MOK_SOURCES
unexport OBJS
unexport SOURCES SUBDIRS
unexport TARGET TARGETS
