VERSION		= 13
ifneq ($(origin RELEASE),undefined)
DASHRELEASE	?= -$(RELEASE)
else
DASHRELEASE	?=
endif

ifeq ($(MAKELEVEL),0)
TOPDIR		?= $(shell pwd)
endif
override TOPDIR	:= $(abspath $(TOPDIR))
VPATH		= $(TOPDIR)

CC		= $(CROSS_COMPILE)gcc
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
DATATARGETDIR	?= $(datadir)/$(PKGNAME)/$(VERSION)$(DASHRELEASE)/$(ARCH_SUFFIX)/
DEBUGINFO	?= $(prefix)/lib/debug/
DEBUGSOURCE	?= $(prefix)/src/debug/
OSLABEL		?= $(EFIDIR)
DEFAULT_LOADER	?= \\\\grub$(ARCH_SUFFIX).efi

ARCH		?= $(shell $(CC) -dumpmachine | cut -f1 -d- | sed s,i[3456789]86,ia32,)
OBJCOPY_GTE224	= $(shell expr `$(OBJCOPY) --version |grep ^"GNU objcopy" | sed 's/^.*\((.*)\|version\) //g' | cut -f1-2 -d.` \>= 2.24)

SUBDIRS		= $(TOPDIR)/Cryptlib $(TOPDIR)/lib

EFI_INCLUDE	:= /usr/include/efi
EFI_INCLUDES	= -nostdinc -I$(TOPDIR)/Cryptlib -I$(TOPDIR)/Cryptlib/Include \
		  -I$(EFI_INCLUDE) -I$(EFI_INCLUDE)/$(ARCH) -I$(EFI_INCLUDE)/protocol \
		  -I$(TOPDIR)/include -iquote $(TOPDIR) -iquote $(shell pwd)

LIB_GCC		= $(shell $(CC) -print-libgcc-file-name)
EFI_LIBS	= -lefi -lgnuefi --start-group Cryptlib/libcryptlib.a Cryptlib/OpenSSL/libopenssl.a --end-group $(LIB_GCC) 

EFI_CRT_OBJS 	= $(EFI_PATH)/crt0-efi-$(ARCH).o
EFI_LDS		= $(TOPDIR)/elf_$(ARCH)_efi.lds

CFLAGS		= -ggdb -O0 -fno-stack-protector -fno-strict-aliasing -fpic \
		  -fshort-wchar -Wall -Wsign-compare -Werror -fno-builtin \
		  -Werror=sign-compare -ffreestanding -std=gnu89 \
		  -I$(shell $(CC) -print-file-name=include) \
		  "-DDEFAULT_LOADER=L\"$(DEFAULT_LOADER)\"" \
		  "-DDEFAULT_LOADER_CHAR=\"$(DEFAULT_LOADER)\"" \
		  $(EFI_INCLUDES)

COMMITID ?= $(shell if [ -d .git ] ; then git log -1 --pretty=format:%H ; elif [ -f commit ]; then cat commit ; else echo commit id not available; fi)

ifneq ($(origin OVERRIDE_SECURITY_POLICY), undefined)
	CFLAGS	+= -DOVERRIDE_SECURITY_POLICY
endif

ifneq ($(origin ENABLE_HTTPBOOT), undefined)
	CFLAGS	+= -DENABLE_HTTPBOOT
endif

ifeq ($(ARCH),x86_64)
	CFLAGS	+= -mno-mmx -mno-sse -mno-red-zone -nostdinc \
		   -maccumulate-outgoing-args \
		   -DEFI_FUNCTION_WRAPPER -DGNU_EFI_USE_MS_ABI \
		   -DNO_BUILTIN_VA_FUNCS -DMDE_CPU_X64 -DPAGE_SIZE=4096
	LIBDIR			?= $(prefix)/lib64
	ARCH_SUFFIX		?= x64
	ARCH_SUFFIX_UPPER	?= X64
	ARCH_LDFLAGS		?=
endif
ifeq ($(ARCH),ia32)
	CFLAGS	+= -mno-mmx -mno-sse -mno-red-zone -nostdinc \
		   -maccumulate-outgoing-args -m32 \
		   -DMDE_CPU_IA32 -DPAGE_SIZE=4096
	LIBDIR			?= $(prefix)/lib
	ARCH_SUFFIX		?= ia32
	ARCH_SUFFIX_UPPER	?= IA32
	ARCH_LDFLAGS		?=
endif
ifeq ($(ARCH),aarch64)
	CFLAGS += -DMDE_CPU_AARCH64 -DPAGE_SIZE=4096 -mstrict-align
	LIBDIR			?= $(prefix)/lib64
	ARCH_SUFFIX		?= aa64
	ARCH_SUFFIX_UPPER	?= AA64
	FORMAT			:= -O binary
	SUBSYSTEM		:= 0xa
	ARCH_LDFLAGS		+= --defsym=EFI_SUBSYSTEM=$(SUBSYSTEM)
endif
ifeq ($(ARCH),arm)
	CFLAGS += -DMDE_CPU_ARM -DPAGE_SIZE=4096 -mstrict-align
	LIBDIR			?= $(prefix)/lib
	ARCH_SUFFIX		?= arm
	ARCH_SUFFIX_UPPER	?= ARM
	FORMAT			:= -O binary
	SUBSYSTEM		:= 0xa
	ARCH_LDFLAGS		+= --defsym=EFI_SUBSYSTEM=$(SUBSYSTEM)
endif

FORMAT		?= --target efi-app-$(ARCH)
EFI_PATH	?= $(LIBDIR)/gnuefi

MMSTEM		?= mm$(ARCH_SUFFIX)
MMNAME		= $(MMSTEM).efi
MMSONAME	= $(MMSTEM).so
FBSTEM		?= fb$(ARCH_SUFFIX)
FBNAME		= $(FBSTEM).efi
FBSONAME	= $(FBSTEM).so
SHIMSTEM	?= shim$(ARCH_SUFFIX)
SHIMNAME	= $(SHIMSTEM).efi
SHIMSONAME	= $(SHIMSTEM).so
SHIMHASHNAME	= $(SHIMSTEM).hash
BOOTEFINAME	?= BOOT$(ARCH_SUFFIX_UPPER).EFI
BOOTCSVNAME	?= BOOT$(ARCH_SUFFIX_UPPER).CSV

CFLAGS += "-DEFI_ARCH=L\"$(ARCH_SUFFIX)\"" "-DDEBUGDIR=L\"/usr/lib/debug/usr/share/shim/$(ARCH_SUFFIX)-$(VERSION)$(DASHRELEASE)/\""

ifneq ($(origin VENDOR_CERT_FILE), undefined)
	CFLAGS += -DVENDOR_CERT_FILE=\"$(VENDOR_CERT_FILE)\"
endif
ifneq ($(origin VENDOR_DBX_FILE), undefined)
	CFLAGS += -DVENDOR_DBX_FILE=\"$(VENDOR_DBX_FILE)\"
endif

LDFLAGS		= --hash-style=sysv -nostdlib -znocombreloc -T $(EFI_LDS) -shared -Bsymbolic -L$(EFI_PATH) -L$(LIBDIR) -LCryptlib -LCryptlib/OpenSSL $(EFI_CRT_OBJS) --build-id=sha1 $(ARCH_LDFLAGS)

TARGETS	= $(SHIMNAME)
TARGETS += $(SHIMNAME).debug $(MMNAME).debug $(FBNAME).debug
ifneq ($(origin ENABLE_SHIM_HASH),undefined)
TARGETS += $(SHIMHASHNAME)
endif
ifneq ($(origin ENABLE_SHIM_CERT),undefined)
TARGETS	+= $(MMNAME).signed $(FBNAME).signed
CFLAGS += -DENABLE_SHIM_CERT
else
TARGETS += $(MMNAME) $(FBNAME)
endif
OBJS	= shim.o netboot.o cert.o replacements.o tpm.o version.o errlog.o
KEYS	= shim_cert.h ocsp.* ca.* shim.crt shim.csr shim.p12 shim.pem shim.key shim.cer
ORIG_SOURCES	= shim.c shim.h netboot.c include/PeImage.h include/wincert.h include/console.h replacements.c replacements.h tpm.c tpm.h version.h errlog.c
MOK_OBJS = MokManager.o PasswordCrypt.o crypt_blowfish.o
ORIG_MOK_SOURCES = MokManager.c shim.h include/console.h PasswordCrypt.c PasswordCrypt.h crypt_blowfish.c crypt_blowfish.h
FALLBACK_OBJS = fallback.o tpm.o
ORIG_FALLBACK_SRCS = fallback.c

ifneq ($(origin ENABLE_HTTPBOOT), undefined)
	OBJS += httpboot.o
	SOURCES += httpboot.c httpboot.h
endif

SOURCES = $(foreach source,$(ORIG_SOURCES),$(TOPDIR)/$(source)) version.c
MOK_SOURCES = $(foreach source,$(ORIG_MOK_SOURCES),$(TOPDIR)/$(source))
FALLBACK_SRCS = $(foreach source,$(ORIG_FALLBACK_SRCS),$(TOPDIR)/$(source))

all: $(TARGETS)

shim.crt:
	$(TOPDIR)/make-certs shim shim@xn--u4h.net all codesign 1.3.6.1.4.1.311.10.3.1 </dev/null

shim.cer: shim.crt
	$(OPENSSL) x509 -outform der -in $< -out $@

.NOTPARALLEL: shim_cert.h
shim_cert.h: shim.cer
	echo "static UINT8 shim_cert[] __attribute__((__unused__)) = {" > $@
	$(HEXDUMP) -v -e '1/1 "0x%02x, "' $< >> $@
	echo "};" >> $@

version.c : $(TOPDIR)/version.c.in
	sed	-e "s,@@VERSION@@,$(VERSION)," \
		-e "s,@@UNAME@@,$(shell uname -a)," \
		-e "s,@@COMMIT@@,$(COMMITID)," \
		< $< > $@

certdb/secmod.db: shim.crt
	-mkdir certdb
	$(PK12UTIL) -d certdb/ -i shim.p12 -W "" -K ""
	$(CERTUTIL) -d certdb/ -A -i shim.crt -n shim -t u

shim.o: $(SOURCES)
ifneq ($(origin ENABLE_SHIM_CERT),undefined)
shim.o: shim_cert.h
endif
shim.o: $(wildcard $(TOPDIR)/*.h)

cert.o : $(TOPDIR)/cert.S
	$(CC) $(CFLAGS) -c -o $@ $<

$(SHIMNAME) : $(SHIMSONAME)
$(MMNAME) : $(MMSONAME)
$(FBNAME) : $(FBSONAME)

$(SHIMSONAME): $(OBJS) Cryptlib/libcryptlib.a Cryptlib/OpenSSL/libopenssl.a lib/lib.a
	$(LD) -o $@ $(LDFLAGS) $^ $(EFI_LIBS)

fallback.o: $(FALLBACK_SRCS)

$(FBSONAME): $(FALLBACK_OBJS) Cryptlib/libcryptlib.a Cryptlib/OpenSSL/libopenssl.a lib/lib.a
	$(LD) -o $@ $(LDFLAGS) $^ $(EFI_LIBS)

MokManager.o: $(MOK_SOURCES)

$(MMSONAME): $(MOK_OBJS) Cryptlib/libcryptlib.a Cryptlib/OpenSSL/libopenssl.a lib/lib.a
	$(LD) -o $@ $(LDFLAGS) $^ $(EFI_LIBS) lib/lib.a

Cryptlib/libcryptlib.a:
	mkdir -p Cryptlib/{Hash,Hmac,Cipher,Rand,Pk,Pem,SysCall}
	$(MAKE) VPATH=$(TOPDIR)/Cryptlib TOPDIR=$(TOPDIR)/Cryptlib -C Cryptlib -f $(TOPDIR)/Cryptlib/Makefile

Cryptlib/OpenSSL/libopenssl.a:
	mkdir -p Cryptlib/OpenSSL/crypto/{x509v3,x509,txt_db,stack,sha,rsa,rc4,rand,pkcs7,pkcs12,pem,ocsp,objects,modes,md5,lhash,kdf,hmac,evp,err,dso,dh,conf,comp,cmac,buffer,bn,bio,async{,/arch},asn1,aes}/
	$(MAKE) VPATH=$(TOPDIR)/Cryptlib/OpenSSL TOPDIR=$(TOPDIR)/Cryptlib/OpenSSL -C Cryptlib/OpenSSL -f $(TOPDIR)/Cryptlib/OpenSSL/Makefile

lib/lib.a:
	if [ ! -d lib ]; then mkdir lib ; fi
	$(MAKE) VPATH=$(TOPDIR)/lib TOPDIR=$(TOPDIR) CFLAGS="$(CFLAGS)" -C lib -f $(TOPDIR)/lib/Makefile

buildid : $(TOPDIR)/buildid.c
	$(CC) -Og -g3 -Wall -Werror -Wextra -o $@ $< -lelf

$(BOOTCSVNAME) :
	@echo Making $@
	@echo "$(SHIMNAME),$(OSLABEL),,This is the boot entry for $(OSLABEL)" | iconv -t UCS-2LE > $@

install-check :
ifeq ($(origin LIBDIR),undefined)
	$(error Architecture $(ARCH) is not a supported build target.)
endif
ifeq ($(origin EFIDIR),undefined)
	$(error EFIDIR must be set to your reserved EFI System Partition subdirectory name)
endif

install-deps : $(TARGETS)
install-deps : $(SHIMNAME).debug $(MMNAME).debug $(FBNAME).debug buildid
install-deps : $(BOOTCSVNAME)

install-debugsource : install-deps
	$(INSTALL) -d -m 0755 $(DESTDIR)/$(DEBUGSOURCE)/$(PKGNAME)-$(VERSION)$(DASHRELEASE)
	find $(TOPDIR) -type f -a '(' -iname '*.c' -o -iname '*.h' -o -iname '*.S' ')' | while read file ; do \
		outfile=$$(echo $${file} | sed -e "s,^$(TOPDIR),,") ; \
		$(INSTALL) -d -m 0755 $(DESTDIR)/$(DEBUGSOURCE)/$(PKGNAME)-$(VERSION)$(DASHRELEASE)/$$(dirname $${outfile}) ; \
		$(INSTALL) -m 0644 $${file} $(DESTDIR)/$(DEBUGSOURCE)/$(PKGNAME)-$(VERSION)$(DASHRELEASE)/$${outfile} ; \
	done

install-debuginfo : install-deps
	$(INSTALL) -d -m 0755 $(DESTDIR)/
	$(INSTALL) -d -m 0755 $(DESTDIR)/$(DEBUGINFO)$(TARGETDIR)/
	@./buildid $(wildcard *.efi.debug) | while read file buildid ; do \
		first=$$(echo $${buildid} | cut -b -2) ; \
		rest=$$(echo $${buildid} | cut -b 3-) ; \
		$(INSTALL) -d -m 0755 $(DESTDIR)/$(DEBUGINFO).build-id/$${first}/ ;\
		$(INSTALL) -m 0644 $${file} $(DESTDIR)/$(DEBUGINFO)$(TARGETDIR) ; \
		ln -s ../../../../..$(DEBUGINFO)$(TARGETDIR)$${file} $(DESTDIR)/$(DEBUGINFO).build-id/$${first}/$${rest}.debug ;\
		ln -s ../../../.build-id/$${first}/$${rest} $(DESTDIR)/$(DEBUGINFO).build-id/$${first}/$${rest} ;\
	done

install : | install-check
install : install-deps install-debuginfo install-debugsource
	$(INSTALL) -d -m 0755 $(DESTDIR)/
	$(INSTALL) -d -m 0700 $(DESTDIR)/$(ESPROOTDIR)
	$(INSTALL) -d -m 0755 $(DESTDIR)/$(EFIBOOTDIR)
	$(INSTALL) -d -m 0755 $(DESTDIR)/$(TARGETDIR)
	$(INSTALL) -m 0644 $(SHIMNAME) $(DESTDIR)/$(EFIBOOTDIR)/$(BOOTEFINAME)
	$(INSTALL) -m 0644 $(SHIMNAME) $(DESTDIR)/$(TARGETDIR)/
	$(INSTALL) -m 0644 $(BOOTCSVNAME) $(DESTDIR)/$(TARGETDIR)/
ifneq ($(origin ENABLE_SHIM_CERT),undefined)
	$(INSTALL) -m 0644 $(FBNAME).signed $(DESTDIR)/$(EFIBOOTDIR)/$(FBNAME)
	$(INSTALL) -m 0644 $(MMNAME).signed $(DESTDIR)/$(EFIBOOTDIR)/$(MMNAME)
	$(INSTALL) -m 0644 $(MMNAME).signed $(DESTDIR)/$(TARGETDIR)/$(MMNAME)
else
	$(INSTALL) -m 0644 $(FBNAME) $(DESTDIR)/$(EFIBOOTDIR)/
	$(INSTALL) -m 0644 $(MMNAME) $(DESTDIR)/$(EFIBOOTDIR)/
	$(INSTALL) -m 0644 $(MMNAME) $(DESTDIR)/$(TARGETDIR)/
endif

install-as-data : install-deps
	$(INSTALL) -d -m 0755 $(DESTDIR)/$(DATATARGETDIR)
	$(INSTALL) -m 0644 $(SHIMNAME) $(DESTDIR)/$(DATATARGETDIR)/
ifneq ($(origin ENABLE_SHIM_HASH),undefined)
	$(INSTALL) -m 0644 $(SHIMHASHNAME) $(DESTDIR)/$(DATATARGETDIR)/
endif
ifneq ($(origin ENABLE_SHIM_CERT),undefined)
	$(INSTALL) -m 0644 $(MMNAME).signed $(DESTDIR)/$(DATATARGETDIR)/$(MMNAME)
	$(INSTALL) -m 0644 $(FBNAME).signed $(DESTDIR)/$(DATATARGETDIR)/$(FBNAME)
else
	$(INSTALL) -m 0644 $(MMNAME) $(DESTDIR)/$(DATATARGETDIR)/$(MMNAME)
	$(INSTALL) -m 0644 $(FBNAME) $(DESTDIR)/$(DATATARGETDIR)/$(FBNAME)
endif

%.efi: %.so
ifneq ($(OBJCOPY_GTE224),1)
	$(error objcopy >= 2.24 is required)
endif
	$(OBJCOPY) -j .text -j .sdata -j .data -j .data.ident \
		-j .dynamic -j .dynsym -j .rel* \
		-j .rela* -j .reloc -j .eh_frame \
		-j .vendor_cert \
		$(FORMAT) $^ $@

ifneq ($(origin ENABLE_SHIM_HASH),undefined)
%.hash : %.efi
	$(PESIGN) -i $< -P -h > $@
endif

%.efi.debug : %.so
ifneq ($(OBJCOPY_GTE224),1)
	$(error objcopy >= 2.24 is required)
endif
	$(OBJCOPY) -j .text -j .sdata -j .data \
		-j .dynamic -j .dynsym -j .rel* \
		-j .rela* -j .reloc -j .eh_frame \
		-j .debug_info -j .debug_abbrev -j .debug_aranges \
		-j .debug_line -j .debug_str -j .debug_ranges \
		-j .note.gnu.build-id \
		$^ $@

ifneq ($(origin ENABLE_SBSIGN),undefined)
%.efi.signed: %.efi shim.key shim.crt
	$(SBSIGN) --key shim.key --cert shim.crt --output $@ $<
else
%.efi.signed: %.efi certdb/secmod.db
	$(PESIGN) -n certdb -i $< -c "shim" -s -o $@ -f
endif

clean: OBJS=$(wildcard *.o)
clean:
	$(MAKE) -C Cryptlib -f $(TOPDIR)/Cryptlib/Makefile clean
	$(MAKE) -C Cryptlib/OpenSSL -f $(TOPDIR)/Cryptlib/OpenSSL/Makefile clean
	$(MAKE) -C lib -f $(TOPDIR)/lib/Makefile clean
	rm -rf $(TARGET) $(OBJS) $(MOK_OBJS) $(FALLBACK_OBJS) $(KEYS) certdb $(BOOTCSVNAME)
	rm -f *.debug *.so *.efi *.efi.* *.tar.* version.c buildid

GITTAG = $(VERSION)

test-archive:
	@rm -rf /tmp/shim-$(VERSION) /tmp/shim-$(VERSION)-tmp
	@mkdir -p /tmp/shim-$(VERSION)-tmp
	@git archive --format=tar $(shell git branch | awk '/^*/ { print $$2 }') | ( cd /tmp/shim-$(VERSION)-tmp/ ; tar x )
	@git diff | ( cd /tmp/shim-$(VERSION)-tmp/ ; patch -s -p1 -b -z .gitdiff )
	@mv /tmp/shim-$(VERSION)-tmp/ /tmp/shim-$(VERSION)/
	@git log -1 --pretty=format:%H > /tmp/shim-$(VERSION)/commit
	@dir=$$PWD; cd /tmp; tar -c --bzip2 -f $$dir/shim-$(VERSION).tar.bz2 shim-$(VERSION)
	@rm -rf /tmp/shim-$(VERSION)
	@echo "The archive is in shim-$(VERSION).tar.bz2"

tag:
	git tag --sign $(GITTAG) refs/heads/master
	git tag -f latest-release $(GITTAG)

archive: tag
	@rm -rf /tmp/shim-$(VERSION) /tmp/shim-$(VERSION)-tmp
	@mkdir -p /tmp/shim-$(VERSION)-tmp
	@git archive --format=tar $(GITTAG) | ( cd /tmp/shim-$(VERSION)-tmp/ ; tar x )
	@mv /tmp/shim-$(VERSION)-tmp/ /tmp/shim-$(VERSION)/
	@git log -1 --pretty=format:%H > /tmp/shim-$(VERSION)/commit
	@dir=$$PWD; cd /tmp; tar -c --bzip2 -f $$dir/shim-$(VERSION).tar.bz2 shim-$(VERSION)
	@rm -rf /tmp/shim-$(VERSION)
	@echo "The archive is in shim-$(VERSION).tar.bz2"

.PHONY : install-deps shim.key

export ARCH CC LD OBJCOPY EFI_INCLUDE
