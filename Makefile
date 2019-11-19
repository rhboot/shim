default : all

NAME		= shim
VERSION		= 15
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

include $(TOPDIR)/include/config.mk
include $(TOPDIR)/include/defaults.mk
include $(TOPDIR)/include/coverity.mk
include $(TOPDIR)/include/scan-build.mk

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
OBJS	= shim.o mok.o netboot.o cert.o replacements.o tpm.o version.o errlog.o httpboot.o
KEYS	= shim_cert.h ocsp.* ca.* shim.crt shim.csr shim.p12 shim.pem shim.key shim.cer
ORIG_SOURCES	= shim.c mok.c netboot.c replacements.c tpm.c errlog.c httpboot.c shim.h version.h $(wildcard include/*.h)
MOK_OBJS = MokManager.o PasswordCrypt.o crypt_blowfish.o errlog.o
ORIG_MOK_SOURCES = MokManager.c PasswordCrypt.c crypt_blowfish.c shim.h $(wildcard include/*.h)
FALLBACK_OBJS = fallback.o tpm.o errlog.o
ORIG_FALLBACK_SRCS = fallback.c

ifeq ($(SOURCE_DATE_EPOCH),)
	UNAME=$(shell uname -s -m -p -i -o)
else
	UNAME=buildhost
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
$(TOPDIR)/shim_cert.h: shim.cer
	echo "static UINT8 shim_cert[] __attribute__((__unused__)) = {" > $@
	$(HEXDUMP) -v -e '1/1 "0x%02x, "' $< >> $@
	echo "};" >> $@

version.c : $(TOPDIR)/version.c.in
	@echo making version.c
	sed	-e "s,@@VERSION@@,$(VERSION)," \
		-e "s,@@UNAME@@,$(UNAME)," \
		-e "s,@@COMMIT@@,$(COMMIT_ID)," \
		-e "s,@@CONFIG@@,$(sort $(CONFIG_ITEMS))," \
		< $< > $@

certdb/secmod.db: shim.crt
	-mkdir certdb
	$(PK12UTIL) -d certdb/ -i shim.p12 -W "" -K ""
	$(CERTUTIL) -d certdb/ -A -i shim.crt -n shim -t u

SHIM_INCLUDES = \
		-I$(TOPDIR)/include \
		-I$(TOPDIR)/Cryptlib/Include \
		-I$(TOPDIR)/Cryptlib \
		-iquote $(TOPDIR) \
		-iquote $(TOPDIR)/Cryptlib \
		-iquote $(OPENSSLDIR) \
		-iquote $(OPENSSLDIR)/crypto/include

%.o : %.c
	$(CC) $(CFLAGS) $(SHIM_INCLUDES) -c -o $@ $<

OPENSSL_LDFLAGS = -Wl,--start-group,Cryptlib/libcryptlib.a,Cryptlib/OpenSSL/libopenssl.a,--end-group

%.so :
	$(CCLD) $(CFLAGS) $(CCLDFLAGS) -o $@ $^ $(OPENSSL_LDFLAGS) $(EFI_LIBS)

shim.o: $(SOURCES)
ifneq ($(origin ENABLE_SHIM_CERT),undefined)
shim.o: shim_cert.h
endif
shim.o: $(wildcard $(TOPDIR)/*.h)
cert.o shim.o shim.h: $(TOPDIR)/config.h

.ONESHELL: $(TOPDIR)/config.h
$(TOPDIR)/config.h :
	@echo making config.h
	(
	echo "#define EFI_ARCH L\"$(ARCH)\""
	echo "#define DEBUGDIR L\"/usr/lib/debug/usr/share/shim/$(ARCH)-$(VERSION)$(DASHRELEASE)/\""
	echo "#define DEFAULT_LOADER L\"$(DEFAULT_LOADER)\""
	echo "#define DEFAULT_LOADER_CHAR \"$(DEFAULT_LOADER)\""
	echo $(CONFIG_VENDOR_CERT)
	echo $(CONFIG_VENDOR_DB)
	echo $(CONFIG_VENDOR_DBX)
	echo $(CONFIG_ENABLE_SHIM_CERT)
	echo $(CONFIG_OVERRIDE_SECURITY_POLICY)
	echo $(CONFIG_ENABLE_HTTPBOOT)
	echo $(CONFIG_REQUIRE_TPM)
	) > $@

# This gives us an ordering dep on everything in src/ and lib/ on the generated
# header files.
$(wildcard *.[chS] lib/*.[chS]) : | config.h shim_cert.h

cert.o : $(TOPDIR)/cert.S
	$(CC) $(CFLAGS) -c -o $@ $<

$(SHIMNAME) : $(SHIMSONAME)
$(MMNAME) : $(MMSONAME)
$(FBNAME) : $(FBSONAME)

$(SHIMSONAME): | Cryptlib/libcryptlib.a Cryptlib/OpenSSL/libopenssl.a
$(SHIMSONAME): $(OBJS) lib/lib.a

fallback.o: $(FALLBACK_SRCS)

$(FBSONAME): | Cryptlib/libcryptlib.a Cryptlib/OpenSSL/libopenssl.a
$(FBSONAME): $(FALLBACK_OBJS) lib/lib.a

MokManager.o: $(MOK_SOURCES)

$(MMSONAME): | Cryptlib/libcryptlib.a Cryptlib/OpenSSL/libopenssl.a
$(MMSONAME): $(MOK_OBJS) lib/lib.a

Cryptlib/libcryptlib.a:
	for i in Hash Hmac Cipher Rand Pk Pem SysCall; do mkdir -p Cryptlib/$$i; done
	$(MAKE) -C Cryptlib -f $(TOPDIR)/Cryptlib/Makefile

Cryptlib/OpenSSL/libopenssl.a:
	for i in fips x509v3 x509 txt_db stack sha rsa rc4 rand pkcs7 pkcs12 pem ocsp objects modes md5 lhash kdf hmac evp err dso dh conf comp cmac buffer bn bio async/arch asn1 aes; do mkdir -p Cryptlib/OpenSSL/crypto/$$i; done
	$(MAKE) -C Cryptlib/OpenSSL -f $(TOPDIR)/Cryptlib/OpenSSL/Makefile

lib/lib.a: | $(TOPDIR)/lib/Makefile $(wildcard $(TOPDIR)/include/*.[ch])
	if [ ! -d lib ]; then mkdir lib ; fi
	$(MAKE) -C lib -f $(TOPDIR)/lib/Makefile lib.a

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
	$(OBJCOPY) -D -j .text -j .sdata -j .data -j .data.ident \
		-j .dynamic -j .dynsym -j .rel* \
		-j .rela* -j .reloc -j .eh_frame \
		-j .vendor_cert \
		$(FORMAT) $^ $@
	# I am tired of wasting my time fighting binutils timestamp code.
	dd conv=notrunc bs=1 count=4 seek=$(TIMESTAMP_LOCATION) if=/dev/zero of=$@

ifneq ($(origin ENABLE_SHIM_HASH),undefined)
%.hash : %.efi
	$(PESIGN) -i $< -P -h > $@
endif

%.efi.debug : %.so
ifneq ($(OBJCOPY_GTE224),1)
	$(error objcopy >= 2.24 is required)
endif
	$(OBJCOPY) -D -j .text -j .sdata -j .data \
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

clean-shim-objs:
	@if [ -d lib ]; then $(MAKE) -C lib -f $(TOPDIR)/lib/Makefile clean ; fi
	@rm -rvf $(TARGETS) *.o $(SHIM_OBJS) $(MOK_OBJS) $(FALLBACK_OBJS) $(KEYS) certdb $(BOOTCSVNAME)
	@rm -vf *.debug *.so *.efi *.efi.* *.tar.* version.c config.h buildid
	@rm -vf Cryptlib/*.[oa] Cryptlib/*/*.[oa]
	@if [[ "$(TOPDIR)" = ..* ]] ; then rmdir Cryptlib || :; fi >/dev/null 2>&1
	@if [ -d .git ] ; then git clean -f -d -e 'Cryptlib/OpenSSL/*'; fi

clean: clean-shim-objs
	$(MAKE) -C Cryptlib -f $(TOPDIR)/Cryptlib/Makefile clean
	$(MAKE) -C Cryptlib/OpenSSL -f $(TOPDIR)/Cryptlib/OpenSSL/Makefile clean

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
