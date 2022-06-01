default : all

NAME		= shim
VERSION		= 15.6
ifneq ($(origin RELEASE),undefined)
DASHRELEASE	?= -$(RELEASE)
else
DASHRELEASE	?=
endif

ifeq ($(MAKELEVEL),0)
TOPDIR		?= $(shell pwd)
endif
ifeq ($(TOPDIR),)
override TOPDIR := $(shell pwd)
endif
override TOPDIR	:= $(abspath $(TOPDIR))
VPATH		= $(TOPDIR)
export TOPDIR

include $(TOPDIR)/Make.rules
include $(TOPDIR)/Make.defaults
include $(TOPDIR)/include/coverity.mk
include $(TOPDIR)/include/scan-build.mk
include $(TOPDIR)/include/fanalyzer.mk

TARGETS	= $(SHIMNAME)
TARGETS += $(SHIMNAME).debug $(MMNAME).debug $(FBNAME).debug
ifneq ($(origin ENABLE_SHIM_HASH),undefined)
TARGETS += $(SHIMHASHNAME)
endif
ifneq ($(origin ENABLE_SHIM_DEVEL),undefined)
CFLAGS += -DENABLE_SHIM_DEVEL
endif
ifneq ($(origin ENABLE_SHIM_CERT),undefined)
TARGETS	+= $(MMNAME).signed $(FBNAME).signed
CFLAGS += -DENABLE_SHIM_CERT
else
TARGETS += $(MMNAME) $(FBNAME)
endif
OBJS	= shim.o globals.o mok.o netboot.o cert.o replacements.o tpm.o version.o errlog.o sbat.o sbat_data.o pe.o httpboot.o csv.o load-options.o
KEYS	= shim_cert.h ocsp.* ca.* shim.crt shim.csr shim.p12 shim.pem shim.key shim.cer
ORIG_SOURCES	= shim.c globals.c mok.c netboot.c replacements.c tpm.c errlog.c sbat.c pe.c httpboot.c shim.h version.h $(wildcard include/*.h) cert.S
MOK_OBJS = MokManager.o PasswordCrypt.o crypt_blowfish.o errlog.o sbat_data.o globals.o
ORIG_MOK_SOURCES = MokManager.c PasswordCrypt.c crypt_blowfish.c shim.h $(wildcard include/*.h)
FALLBACK_OBJS = fallback.o tpm.o errlog.o sbat_data.o globals.o
ORIG_FALLBACK_SRCS = fallback.c
SBATPATH = $(TOPDIR)/data/sbat.csv

ifeq ($(SOURCE_DATE_EPOCH),)
	UNAME=$(shell uname -s -m -p -i -o)
else
	UNAME=buildhost
endif

SOURCES = $(foreach source,$(ORIG_SOURCES),$(TOPDIR)/$(source)) version.c
MOK_SOURCES = $(foreach source,$(ORIG_MOK_SOURCES),$(TOPDIR)/$(source))
FALLBACK_SRCS = $(foreach source,$(ORIG_FALLBACK_SRCS),$(TOPDIR)/$(source))

ifneq ($(origin FALLBACK_VERBOSE), undefined)
	CFLAGS += -DFALLBACK_VERBOSE
endif

ifneq ($(origin FALLBACK_NONINTERACTIVE), undefined)
	CFLAGS += -DFALLBACK_NONINTERACTIVE
endif

ifneq ($(origin FALLBACK_VERBOSE_WAIT), undefined)
	CFLAGS += -DFALLBACK_VERBOSE_WAIT=$(FALLBACK_VERBOSE_WAIT)
endif

all: confcheck $(TARGETS)

confcheck:
ifneq ($(origin EFI_PATH),undefined)
	$(error EFI_PATH is no longer supported, you must build using the supplied copy of gnu-efi)
endif

update :
	git submodule update --init --recursive

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
		-e "s,@@UNAME@@,$(UNAME)," \
		-e "s,@@COMMIT@@,$(COMMIT_ID)," \
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

sbat.%.csv : data/sbat.%.csv
	$(DOS2UNIX) $(D2UFLAGS) $< $@
	tail -c1 $@ | read -r _ || echo >> $@ # ensure a trailing newline

VENDOR_SBATS := $(sort $(foreach x,$(wildcard $(TOPDIR)/data/sbat.*.csv data/sbat.*.csv),$(notdir $(x))))

sbat_data.o : | $(SBATPATH) $(VENDOR_SBATS)
sbat_data.o : /dev/null
	$(CC) $(CFLAGS) -x c -c -o $@ $<
	$(OBJCOPY) --add-section .sbat=$(SBATPATH) \
		--set-section-flags .sbat=contents,alloc,load,readonly,data \
		$@
	$(foreach vs,$(VENDOR_SBATS),$(call add-vendor-sbat,$(vs),$@))

$(SHIMNAME) : $(SHIMSONAME) post-process-pe
$(MMNAME) : $(MMSONAME) post-process-pe
$(FBNAME) : $(FBSONAME) post-process-pe
$(SHIMNAME) $(MMNAME) $(FBNAME) : | post-process-pe

LIBS = Cryptlib/libcryptlib.a \
       Cryptlib/OpenSSL/libopenssl.a \
       lib/lib.a \
       gnu-efi/$(ARCH_GNUEFI)/lib/libefi.a \
       gnu-efi/$(ARCH_GNUEFI)/gnuefi/libgnuefi.a

$(SHIMSONAME): $(OBJS) $(LIBS)
	$(LD) -o $@ $(LDFLAGS) $^ $(EFI_LIBS) lib/lib.a

fallback.o: $(FALLBACK_SRCS)

$(FBSONAME): $(FALLBACK_OBJS) $(LIBS)
	$(LD) -o $@ $(LDFLAGS) $^ $(EFI_LIBS) lib/lib.a

MokManager.o: $(MOK_SOURCES)

$(MMSONAME): $(MOK_OBJS) $(LIBS)
	$(LD) -o $@ $(LDFLAGS) $^ $(EFI_LIBS) lib/lib.a

gnu-efi/$(ARCH_GNUEFI)/gnuefi/libgnuefi.a gnu-efi/$(ARCH_GNUEFI)/lib/libefi.a: CFLAGS+=-DGNU_EFI_USE_EXTERNAL_STDARG
gnu-efi/$(ARCH_GNUEFI)/gnuefi/libgnuefi.a gnu-efi/$(ARCH_GNUEFI)/lib/libefi.a:
	mkdir -p gnu-efi/lib gnu-efi/gnuefi
	$(MAKE) -C gnu-efi \
		COMPILER="$(COMPILER)" \
		CCC_CC="$(COMPILER)" \
		CC="$(CC)" \
		ARCH=$(ARCH_GNUEFI) \
		TOPDIR=$(TOPDIR)/gnu-efi \
		-f $(TOPDIR)/gnu-efi/Makefile \
		lib gnuefi inc

Cryptlib/libcryptlib.a:
	for i in Hash Hmac Cipher Rand Pk Pem SysCall; do mkdir -p Cryptlib/$$i; done
	$(MAKE) TOPDIR=$(TOPDIR) VPATH=$(TOPDIR)/Cryptlib -C Cryptlib -f $(TOPDIR)/Cryptlib/Makefile

Cryptlib/OpenSSL/libopenssl.a:
	for i in x509v3 x509 txt_db stack sha rsa rc4 rand pkcs7 pkcs12 pem ocsp objects modes md5 lhash kdf hmac evp err dso dh conf comp cmac buffer bn bio async/arch asn1 aes; do mkdir -p Cryptlib/OpenSSL/crypto/$$i; done
	$(MAKE) TOPDIR=$(TOPDIR) VPATH=$(TOPDIR)/Cryptlib/OpenSSL -C Cryptlib/OpenSSL -f $(TOPDIR)/Cryptlib/OpenSSL/Makefile

lib/lib.a: | $(TOPDIR)/lib/Makefile $(wildcard $(TOPDIR)/include/*.[ch])
	mkdir -p lib
	$(MAKE) VPATH=$(TOPDIR)/lib TOPDIR=$(TOPDIR) -C lib -f $(TOPDIR)/lib/Makefile

post-process-pe : $(TOPDIR)/post-process-pe.c
	$(HOSTCC) -std=gnu11 -Og -g3 -Wall -Wextra -Wno-missing-field-initializers -Werror -o $@ $<

buildid : $(TOPDIR)/buildid.c
	$(HOSTCC) -I/usr/include -Og -g3 -Wall -Werror -Wextra -o $@ $< -lelf

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
	$(INSTALL) -m 0644 $(BOOTCSVNAME) $(DESTDIR)/$(DATATARGETDIR)/
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
	$(OBJCOPY) -D -j .text -j .sdata -j .data -j .data.ident \
		-j .dynamic -j .rodata -j .rel* \
		-j .rela* -j .dyn -j .reloc -j .eh_frame \
		-j .vendor_cert -j .sbat \
		$(FORMAT) $< $@
	./post-process-pe -vv $@

ifneq ($(origin ENABLE_SHIM_HASH),undefined)
%.hash : %.efi
	$(PESIGN) -i $< -P -h > $@
endif

%.efi.debug : %.so
ifneq ($(OBJCOPY_GTE224),1)
	$(error objcopy >= 2.24 is required)
endif
	$(OBJCOPY) -D -j .text -j .sdata -j .data \
		-j .dynamic -j .rodata -j .rel* \
		-j .rela* -j .dyn -j .reloc -j .eh_frame -j .sbat \
		-j .debug_info -j .debug_abbrev -j .debug_aranges \
		-j .debug_line -j .debug_str -j .debug_ranges \
		-j .note.gnu.build-id \
		$< $@

ifneq ($(origin ENABLE_SBSIGN),undefined)
%.efi.signed: %.efi shim.key shim.crt
	@$(SBSIGN) \
		--key shim.key \
		--cert shim.crt \
		--output $@ $<
else
%.efi.signed: %.efi certdb/secmod.db
	$(PESIGN) -n certdb -i $< -c "shim" -s -o $@ -f
endif

test test-clean test-coverage test-lto :
	@make -f $(TOPDIR)/include/test.mk \
		COMPILER="$(COMPILER)" \
		CROSS_COMPILE="$(CROSS_COMPILE)" \
		CLANG_WARNINGS="$(CLANG_WARNINGS)" \
		ARCH_DEFINES="$(ARCH_DEFINES)" \
		EFI_INCLUDES="$(EFI_INCLUDES)" \
		test-clean $@

$(patsubst %.c,%,$(wildcard test-*.c)) :
	@make -f $(TOPDIR)/include/test.mk EFI_INCLUDES="$(EFI_INCLUDES)" ARCH_DEFINES="$(ARCH_DEFINES)" $@

.PHONY : $(patsubst %.c,%,$(wildcard test-*.c)) test

clean-test-objs:
	@make -f $(TOPDIR)/include/test.mk EFI_INCLUDES="$(EFI_INCLUDES)" ARCH_DEFINES="$(ARCH_DEFINES)" clean

clean-gnu-efi:
	@if [ -d gnu-efi ] ; then \
		$(MAKE) -C gnu-efi \
			CC="$(CC)" \
			HOSTCC="$(HOSTCC)" \
			COMPILER="$(COMPILER)" \
			ARCH=$(ARCH_GNUEFI) \
			TOPDIR=$(TOPDIR)/gnu-efi \
			-f $(TOPDIR)/gnu-efi/Makefile \
			clean ; \
	fi

clean-lib-objs:
	@if [ -d lib ] ; then \
		$(MAKE) -C lib TOPDIR=$(TOPDIR) -f $(TOPDIR)/lib/Makefile clean ; \
	fi

clean-shim-objs:
	@rm -rvf $(TARGET) *.o $(SHIM_OBJS) $(MOK_OBJS) $(FALLBACK_OBJS) $(KEYS) certdb $(BOOTCSVNAME)
	@rm -vf *.debug *.so *.efi *.efi.* *.tar.* version.c buildid post-process-pe
	@rm -vf Cryptlib/*.[oa] Cryptlib/*/*.[oa]
	@if [ -d .git ] ; then git clean -f -d -e 'Cryptlib/OpenSSL/*'; fi

clean-openssl-objs:
	@if [ -d Cryptlib/OpenSSL ] ; then \
		$(MAKE) -C Cryptlib/OpenSSL -f $(TOPDIR)/Cryptlib/OpenSSL/Makefile clean ; \
	fi

clean-cryptlib-objs:
	@if [ -d Cryptlib ] ; then \
		$(MAKE) -C Cryptlib -f $(TOPDIR)/Cryptlib/Makefile clean ; \
	fi

clean: clean-shim-objs clean-test-objs clean-gnu-efi clean-openssl-objs clean-cryptlib-objs clean-lib-objs

GITTAG = $(VERSION)

test-archive:
	@./make-archive $(if $(call get-config,shim.origin),--origin "$(call get-config,shim.origin)") --test "$(VERSION)"

tag:
	git tag --sign $(GITTAG) refs/heads/main
	git tag -f latest-release $(GITTAG)

archive: tag
	@./make-archive $(if $(call get-config,shim.origin),--origin "$(call get-config,shim.origin)") --release "$(VERSION)" "$(GITTAG)" "shim-$(GITTAG)"

.PHONY : install-deps shim.key

export ARCH CC CROSS_COMPILE LD OBJCOPY EFI_INCLUDE EFI_INCLUDES OPTIMIZATIONS
export FEATUREFLAGS WARNFLAGS WERRFLAGS
