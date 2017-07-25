VERSION		= 12
RELEASE		:=
ifneq ($(RELEASE),"")
	RELEASE:="-$(RELEASE)"
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
PK12UTIL	?= pk12util
CERTUTIL	?= certutil
PESIGN		?= pesign

ARCH		?= $(shell $(CC) -dumpmachine | cut -f1 -d- | sed s,i[3456789]86,ia32,)
OBJCOPY_GTE224  = $(shell expr `$(OBJCOPY) --version |grep ^"GNU objcopy" | sed 's/^.*\((.*)\|version\) //g' | cut -f1-2 -d.` \>= 2.24)

SUBDIRS		= $(TOPDIR)/Cryptlib $(TOPDIR)/lib

EFI_INCLUDE	:= /usr/include/efi
EFI_INCLUDES	= -nostdinc -I$(TOPDIR)/Cryptlib -I$(TOPDIR)/Cryptlib/Include \
		  -I$(EFI_INCLUDE) -I$(EFI_INCLUDE)/$(ARCH) -I$(EFI_INCLUDE)/protocol \
		  -I$(TOPDIR)/include -iquote $(TOPDIR) -iquote $(shell pwd)

LIB_GCC		= $(shell $(CC) -print-libgcc-file-name)
EFI_LIBS	= -lefi -lgnuefi --start-group Cryptlib/libcryptlib.a Cryptlib/OpenSSL/libopenssl.a --end-group $(LIB_GCC) 

EFI_CRT_OBJS 	= $(EFI_PATH)/crt0-efi-$(ARCH).o
EFI_LDS		= $(TOPDIR)/elf_$(ARCH)_efi.lds

DEFAULT_LOADER	:= \\\\grub.efi
CFLAGS		= -ggdb -O0 -fno-stack-protector -fno-strict-aliasing -fpic \
		  -fshort-wchar -Wall -Wsign-compare -Werror -fno-builtin \
		  -Werror=sign-compare -ffreestanding -std=gnu89 \
		  -I$(shell $(CC) -print-file-name=include) \
		  "-DDEFAULT_LOADER=L\"$(DEFAULT_LOADER)\"" \
		  "-DDEFAULT_LOADER_CHAR=\"$(DEFAULT_LOADER)\"" \
		  $(EFI_INCLUDES)
SHIMNAME	= shim
MMNAME		= MokManager
FBNAME		= fallback

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
		-DNO_BUILTIN_VA_FUNCS \
		-DMDE_CPU_X64 "-DEFI_ARCH=L\"x64\"" -DPAGE_SIZE=4096 \
		"-DDEBUGDIR=L\"/usr/lib/debug/usr/share/shim/x64-$(VERSION)$(RELEASE)/\""
	MMNAME	= mmx64
	FBNAME	= fbx64
	SHIMNAME= shimx64
	EFI_PATH:=/usr/lib64/gnuefi
	LIB_PATH:=/usr/lib64

endif
ifeq ($(ARCH),ia32)
	CFLAGS	+= -mno-mmx -mno-sse -mno-red-zone -nostdinc \
		-maccumulate-outgoing-args -m32 \
		-DMDE_CPU_IA32 "-DEFI_ARCH=L\"ia32\"" -DPAGE_SIZE=4096 \
		"-DDEBUGDIR=L\"/usr/lib/debug/usr/share/shim/ia32-$(VERSION)$(RELEASE)/\""
	MMNAME	= mmia32
	FBNAME	= fbia32
	SHIMNAME= shimia32
	EFI_PATH:=/usr/lib/gnuefi
	LIB_PATH:=/usr/lib
endif
ifeq ($(ARCH),aarch64)
	CFLAGS += -DMDE_CPU_AARCH64 "-DEFI_ARCH=L\"aa64\"" -DPAGE_SIZE=4096 \
		"-DDEBUGDIR=L\"/usr/lib/debug/usr/share/shim/aa64-$(VERSION)$(RELEASE)/\""
	MMNAME	= mmaa64
	FBNAME	= fbaa64
	SHIMNAME= shimaa64
	EFI_PATH:=/usr/lib64/gnuefi
	LIB_PATH:=/usr/lib64
endif

ifneq ($(origin VENDOR_CERT_FILE), undefined)
	CFLAGS += -DVENDOR_CERT_FILE=\"$(VENDOR_CERT_FILE)\"
endif
ifneq ($(origin VENDOR_DBX_FILE), undefined)
	CFLAGS += -DVENDOR_DBX_FILE=\"$(VENDOR_DBX_FILE)\"
endif

LDFLAGS		= --hash-style=sysv -nostdlib -znocombreloc -T $(EFI_LDS) -shared -Bsymbolic -L$(EFI_PATH) -L$(LIB_PATH) -LCryptlib -LCryptlib/OpenSSL $(EFI_CRT_OBJS) --build-id=sha1

TARGET	= $(SHIMNAME).efi $(MMNAME).efi.signed $(FBNAME).efi.signed
OBJS	= shim.o netboot.o cert.o replacements.o tpm.o version.o
KEYS	= shim_cert.h ocsp.* ca.* shim.crt shim.csr shim.p12 shim.pem shim.key shim.cer
ORIG_SOURCES	= shim.c shim.h netboot.c include/PeImage.h include/wincert.h include/console.h replacements.c replacements.h tpm.c tpm.h version.h
MOK_OBJS = MokManager.o PasswordCrypt.o crypt_blowfish.o
ORIG_MOK_SOURCES = MokManager.c shim.h include/console.h PasswordCrypt.c PasswordCrypt.h crypt_blowfish.c crypt_blowfish.h
FALLBACK_OBJS = fallback.o
ORIG_FALLBACK_SRCS = fallback.c

ifneq ($(origin ENABLE_HTTPBOOT), undefined)
	OBJS += httpboot.o
	SOURCES += httpboot.c httpboot.h
endif

SOURCES = $(foreach source,$(ORIG_SOURCES),$(TOPDIR)/$(source)) version.c
MOK_SOURCES = $(foreach source,$(ORIG_MOK_SOURCES),$(TOPDIR)/$(source))
FALLBACK_SRCS = $(foreach source,$(ORIG_FALLBACK_SRCS),$(TOPDIR)/$(source))

all: $(TARGET)

shim.crt:
	$(TOPDIR)/make-certs shim shim@xn--u4h.net all codesign 1.3.6.1.4.1.311.10.3.1 </dev/null

shim.cer: shim.crt
	$(OPENSSL) x509 -outform der -in $< -out $@

shim_cert.h: shim.cer
	echo "static UINT8 shim_cert[] = {" > $@
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

shim.o: $(SOURCES) shim_cert.h
shim.o: $(wildcard $(TOPDIR)/*.h *.h)

cert.o : $(TOPDIR)/cert.S
	$(CC) $(CFLAGS) -c -o $@ $<

$(SHIMNAME).so: $(OBJS) Cryptlib/libcryptlib.a Cryptlib/OpenSSL/libopenssl.a lib/lib.a
	$(LD) -o $@ $(LDFLAGS) $^ $(EFI_LIBS)

fallback.o: $(FALLBACK_SRCS)

$(FBNAME).so: $(FALLBACK_OBJS) Cryptlib/libcryptlib.a Cryptlib/OpenSSL/libopenssl.a lib/lib.a
	$(LD) -o $@ $(LDFLAGS) $^ $(EFI_LIBS)

MokManager.o: $(MOK_SOURCES)

$(MMNAME).so: $(MOK_OBJS) Cryptlib/libcryptlib.a Cryptlib/OpenSSL/libopenssl.a lib/lib.a
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

ifeq ($(ARCH),aarch64)
FORMAT		:= -O binary
SUBSYSTEM	:= 0xa
LDFLAGS		+= --defsym=EFI_SUBSYSTEM=$(SUBSYSTEM)
endif

ifeq ($(ARCH),arm)
FORMAT		:= -O binary
SUBSYSTEM	:= 0xa
LDFLAGS		+= --defsym=EFI_SUBSYSTEM=$(SUBSYSTEM)
endif

FORMAT		?= --target efi-app-$(ARCH)

%.efi: %.so
ifneq ($(OBJCOPY_GTE224),1)
	$(error objcopy >= 2.24 is required)
endif
	$(OBJCOPY) -j .text -j .sdata -j .data -j .data.ident \
		-j .dynamic -j .dynsym  -j .rel* \
		-j .rela* -j .reloc -j .eh_frame \
		-j .vendor_cert \
		$(FORMAT)  $^ $@
	$(OBJCOPY) -j .text -j .sdata -j .data \
		-j .dynamic -j .dynsym  -j .rel* \
		-j .rela* -j .reloc -j .eh_frame \
		-j .debug_info -j .debug_abbrev -j .debug_aranges \
		-j .debug_line -j .debug_str -j .debug_ranges \
		-j .note.gnu.build-id \
		$(FORMAT) $^ $@.debug

%.efi.signed: %.efi certdb/secmod.db
	$(PESIGN) -n certdb -i $< -c "shim" -s -o $@ -f

clean:
	$(MAKE) -C Cryptlib -f $(TOPDIR)/Cryptlib/Makefile clean
	$(MAKE) -C Cryptlib/OpenSSL -f $(TOPDIR)/Cryptlib/OpenSSL/Makefile clean
	$(MAKE) -C lib -f $(TOPDIR)/lib/Makefile clean
	rm -rf $(TARGET) $(OBJS) $(MOK_OBJS) $(FALLBACK_OBJS) $(KEYS) certdb
	rm -f *.debug *.so *.efi *.tar.* version.c

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

export ARCH CC LD OBJCOPY EFI_INCLUDE
