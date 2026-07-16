default : all

NAME		= shim
VERSION		= 16.1
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

OBJS	= shim.o \
	  backtrace.o \
	  cert.o \
	  csv.o \
	  dp.o \
	  errlog.o \
	  hexdump.o \
	  httpboot.o \
	  globals.o \
	  load-options.o \
	  loader-proto.o \
	  memattrs.o \
	  mok.o \
	  netboot.o \
	  pe.o \
	  pe-relocate.o \
	  sbat.o \
	  sbat_data.o \
	  sbat_var.o \
	  stack-protector.o \
	  time.o \
	  tpm.o \
	  utils.o \
	  verify.o \
	  version.o \

KEYS	= shim_cert.h \
	  ocsp.* \
	  ca.* \
	  shim.crt \
	  shim.csr \
	  shim.p12 \
	  shim.pem \
	  shim.key \
	  shim.cer \

ORIG_SOURCES	= shim.c \
		  backtrace.c \
		  cert.S \
		  csv.c \
		  dp.c \
		  errlog.c \
		  hexdump.c \
		  httpboot.c \
		  globals.c \
		  load-options.c \
		  loader-proto.c \
		  memattrs.c \
		  mok.c \
		  netboot.c \
		  pe.c \
		  pe-relocate.c \
		  sbat.c \
		  sbat_var.S \
		  stack-protector.c \
		  shim.h \
		  time.c \
		  tpm.c \
		  utils.c \
		  verify.c \
		  version.h \
		  $(wildcard include/*.h) \

MOK_OBJS = MokManager.o \
	   crypt_blowfish.o \
	   backtrace.o \
	   dp.o \
	   errlog.o \
	   globals.o \
	   hexdump.o \
	   PasswordCrypt.o \
	   sbat_data.o \
	   stack-protector.o \
	   time.o \
	   utils.o \

ORIG_MOK_SOURCES = MokManager.c \
		   crypt_blowfish.c \
		   PasswordCrypt.c \
		   shim.h \
		   $(wildcard include/*.h) \

FALLBACK_OBJS = fallback.o \
		backtrace.o \
		errlog.o \
		globals.o \
		hexdump.o \
		sbat_data.o \
		stack-protector.o \
		time.o \
		tpm.o \
		utils.o

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

all: confcheck certcheck $(TARGETS)

confcheck:
ifneq ($(origin EFI_PATH),undefined)
	$(error EFI_PATH is no longer supported, you must build using the supplied copy of gnu-efi)
endif

certcheck:
ifneq ($(origin VENDOR_CERT_FILE), undefined)
	@if grep -q "BEGIN" $(VENDOR_CERT_FILE); then \
		echo "$(VENDOR_CERT_FILE) is PEM-format, convert to DER!"; \
		exit 1; \
	fi
endif

compile_commands.json : Makefile Make.rules Make.defaults 
	make clean
	bear -- make COMPILER=clang WARNFLAGS+="-Wno-#warnings" test all
	sed -i \
		-e 's/"-maccumulate-outgoing-args",//g' \
		$@

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
# Both of these need to be here so that when TOPDIR is unset, make isn't trying
# to match against ./sbat_var.S, which isn't a target it will ever try to build.
$(TOPDIR)/sbat_var.S sbat_var.S: generated_sbat_var_defs.h
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
       gnu-efi/lib/libefi.a \
       gnu-efi/gnuefi/libgnuefi.a

$(SHIMSONAME): $(OBJS) $(LIBS)
	$(LD) -o $@ $(LDFLAGS) $^ $(EFI_LIBS) lib/lib.a

fallback.o: $(FALLBACK_SRCS)

$(FBSONAME): $(FALLBACK_OBJS) $(LIBS)
	$(LD) -o $@ $(LDFLAGS) $^ $(EFI_LIBS) lib/lib.a

MokManager.o: $(MOK_SOURCES)

$(MMSONAME): $(MOK_OBJS) $(LIBS)
	$(LD) -o $@ $(LDFLAGS) $^ $(EFI_LIBS) lib/lib.a

gnu-efi/gnuefi/libgnuefi.a gnu-efi/lib/libefi.a:
	mkdir -p gnu-efi/lib gnu-efi/gnuefi
	$(MAKE) -C gnu-efi \
		COMPILER="$(COMPILER)" \
		CCC_CC="$(COMPILER)" \
		CC="$(CC)" \
		ARCH=$(ARCH_GNUEFI) \
		NO_GLIBC=1 \
		TOPDIR=$(TOPDIR)/gnu-efi \
		VPATH=$(TOPDIR)/gnu-efi \
		OBJDIR=. \
		-f $(TOPDIR)/gnu-efi/Makefile \
		lib gnuefi inc $(IGNORE_COMPILER_ERRORS)

CRYPTLIB_DIRS = Hash \
		Hmac \
		Cipher \
		Pem \
		Pk \
		Rand \
		SysCall \

Cryptlib/libcryptlib.a:
	for i in $(CRYPTLIB_DIRS) ; do mkdir -p Cryptlib/$$i; done
	$(MAKE) TOPDIR=$(TOPDIR) VPATH=$(TOPDIR)/Cryptlib -C Cryptlib -f $(TOPDIR)/Cryptlib/Makefile $(IGNORE_COMPILER_ERRORS)

CRYPTO_DIRS = aes \
	      asn1 \
	      async/arch \
	      bio \
	      bn \
	      buffer \
	      cmac \
	      cms \
	      comp \
	      conf \
	      dh \
	      dso \
	      ec \
	      ec/curve448/arch_64 \
	      encode_decode \
	      err \
	      evp \
	      ffc \
	      hashtable \
	      hmac \
	      hpke \
	      http \
	      kdf \
	      lhash \
	      md5 \
	      ml_dsa \
	      ml_kem \
	      modes \
	      objects \
	      ocsp \
	      pem \
	      pkcs12 \
	      pkcs7 \
	      property \
	      provider \
	      rand \
	      rc4 \
	      rsa \
	      sha \
	      stack \
	      txt_db \
	      ui \
	      x509 \
	      x509v3 \

PROVIDERS_DIRS = common/der \
		 implementations/asymciphers \
		 implementations/ciphers \
		 implementations/digests \
		 implementations/encode_decode \
		 implementations/kem \
		 implementations/keymgmt \
		 implementations/macs \
		 implementations/rands/seeding \
		 implementations/signature \
		 implementations/skeymgmt \

STUB_DIRS = / \

Cryptlib/OpenSSL/libopenssl.a:
	for i in $(CRYPTO_DIRS) ; do mkdir -p Cryptlib/OpenSSL/crypto/$$i; done
	for i in $(PROVIDERS_DIRS) ; do mkdir -p Cryptlib/OpenSSL/providers/$$i; done
	for i in $(STUB_DIRS) ; do mkdir -p Cryptlib/OpenSSL/stub/$$i; done
	$(MAKE) TOPDIR=$(TOPDIR) VPATH=$(TOPDIR)/Cryptlib/OpenSSL -C Cryptlib/OpenSSL -f $(TOPDIR)/Cryptlib/OpenSSL/Makefile $(IGNORE_COMPILER_ERRORS)

lib/lib.a: | $(TOPDIR)/lib/Makefile $(wildcard $(TOPDIR)/include/*.[ch])
	mkdir -p lib
	$(MAKE) VPATH=$(TOPDIR)/lib TOPDIR=$(TOPDIR) -C lib -f $(TOPDIR)/lib/Makefile $(IGNORE_COMPILER_ERRORS)

post-process-pe : $(TOPDIR)/post-process-pe.c
	$(HOSTCC) -std=gnu11 -Og -g3 -Wall -Wextra -Wno-missing-field-initializers -Werror -o $@ $<

generate_sbat_var_defs: $(TOPDIR)/generate_sbat_var_defs.c
	$(HOSTCC) -std=gnu11 -Og -g3 -Wall -Wextra -Wno-missing-field-initializers -Werror -o $@ $<

.NOTPARALLEL: generated_sbat_var_defs.h
generated_sbat_var_defs.h: generate_sbat_var_defs
	./generate_sbat_var_defs $(TOPDIR) > $@

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
		-j .rela* -j .dyn* -j .reloc -j .eh_frame \
		-j .vendor_cert -j .sbat -j .sbatlevel \
		--file-alignment 0x1000 \
		--section-alignment $(ARCH_SECTION_ALIGNMENT) \
		$(FORMAT) $< $@
	./post-process-pe $(POST_PROCESS_PE_FLAGS) $@

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
		-j .rela* -j .dyn* -j .reloc -j .eh_frame -j .sbat \
		-j .sbatlevel \
		-j .debug_info -j .debug_abbrev -j .debug_aranges \
		-j .debug_line -j .debug_str -j .debug_ranges \
		-j .note.gnu.build-id \
		--file-alignment 0x1000 \
		--section-alignment $(ARCH_SECTION_ALIGNMENT) \
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

fuzz:
	@make -f $(TOPDIR)/include/fuzz.mk \
		COMPILER="$(COMPILER)" \
		CROSS_COMPILE="$(CROSS_COMPILE)" \
		CLANG_WARNINGS="$(CLANG_WARNINGS)" \
		ARCH_DEFINES="$(ARCH_DEFINES)" \
		EFI_INCLUDES="$(EFI_INCLUDES)" \
		MAX_FUZZ_TIME=60 \
		$@

test test-coverage test-lto : generated_sbat_var_defs.h
	@make -f $(TOPDIR)/include/test.mk \
		COMPILER="$(COMPILER)" \
		CROSS_COMPILE="$(CROSS_COMPILE)" \
		CLANG_WARNINGS="$(CLANG_WARNINGS)" \
		ARCH_DEFINES="$(ARCH_DEFINES)" \
		EFI_INCLUDES="$(EFI_INCLUDES)" \
		$@

fuzz-clean:
	@rm -vf random.bin libefi-test.a $(wildcard *-corpus/fuzz*.log)

test-clean:
	@rm -vf test-random.h libefi-test.a
	@rm -vf *.gcda *.gcno *.gcov vgcore.*

$(patsubst %.c,%,$(wildcard fuzz-*.c)) :
	@make -f $(TOPDIR)/include/fuzz.mk EFI_INCLUDES="$(EFI_INCLUDES)" ARCH_DEFINES="$(ARCH_DEFINES)" $@

$(patsubst %.c,%,$(wildcard test-*.c)) :
	@make -f $(TOPDIR)/include/test.mk EFI_INCLUDES="$(EFI_INCLUDES)" ARCH_DEFINES="$(ARCH_DEFINES)" $@

clean-fuzz-objs:
	@find . -type f -a -perm /111 -a -iname 'fuzz-*' -print -delete

clean-test-objs:
	@find . -type f -a -perm /111 -a -iname 'test-*' -print -delete

.PHONY : $(patsubst %.c,%,$(wildcard fuzz-*.c)) fuzz
.PHONY : $(patsubst %.c,%,$(wildcard test-*.c)) test

clean-gnu-efi:
	@if [ -d gnu-efi ] ; then \
		$(MAKE) -C gnu-efi \
			CC="$(CC)" \
			HOSTCC="$(HOSTCC)" \
			COMPILER="$(COMPILER)" \
			ARCH=$(ARCH_GNUEFI) \
			TOPDIR=$(TOPDIR)/gnu-efi \
			VPATH=$(TOPDIR)/gnu-efi \
			OBJDIR=. \
			-f $(TOPDIR)/gnu-efi/Makefile \
			clean ; \
	fi

clean-lib-objs:
	@if [ -d lib ] ; then \
		$(MAKE) -C lib TOPDIR=$(TOPDIR) -f $(TOPDIR)/lib/Makefile clean ; \
	fi

clean-shim-objs:
	@rm -rvf $(TARGET) *.o $(SHIM_OBJS) $(MOK_OBJS) $(FALLBACK_OBJS) $(KEYS) certdb $(BOOTCSVNAME)
	@rm -vf *.debug *.so *.efi *.efi.* *.tar.* version.c buildid post-process-pe compile_commands.json
	@rm -vf generate_sbat_var_defs generated_sbat_var_defs.h
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

clean: clean-shim-objs clean-fuzz-objs clean-test-objs clean-gnu-efi clean-openssl-objs clean-cryptlib-objs clean-lib-objs

GITTAG = $(shell echo $(VERSION) | sed 's/~/-/g')

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
unexport CFLAGS CPPFLAGS LDFLAGS
