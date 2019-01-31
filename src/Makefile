
ORIG_SHIM_SOURCES = shim.c mok.c netboot.c replacements.c tpm.c errlog.c version.c
ORIG_MOK_SOURCES = MokManager.c PasswordCrypt.c crypt_blowfish.c
ORIG_FALLBACK_SOURCES = fallback.c

ifneq ($(origin ENABLE_HTTPBOOT), undefined)
	ORIG_SHIM_SOURCES += httpboot.c
endif

SHIM_SOURCES := $(foreach x,$(ORIG_SHIM_SOURCES),src/$(x))
SHIM_OBJECTS := $(foreach x,$(SHIM_SOURCES),$(patsubst %.c,%.o,$(x)))
MOK_SOURCES := $(foreach x,$(ORIG_MOK_SOURCES),src/$(x))
MOK_OBJECTS := $(foreach x,$(MOK_SOURCES),$(patsubst %.c,%.o,$(x)))
FALLBACK_SOURCES := $(foreach x,$(ORIG_FALLBACK_SOURCES),src/$(x))
FALLBACK_OBJECTS := $(foreach x,$(FALLBACK_SOURCES),$(patsubst %.c,%.o,$(x)))

$(foreach x,$(SHIM_SOURCES),$(eval $(call object-template,src/,$(x),$$(SHIM_INCLUDES))))
$(foreach x,$(MOK_SOURCES),$(eval $(call object-template,src/,$(x),$$(MOK_INCLUDES))))
$(foreach x,$(FALLBACK_SOURCES),$(eval $(call object-template,src/,$(x),$$(FALLBACK_INCLUDES))))

SHIM_SOURCES += src/cert.S
SHIM_OBJECTS += src/cert.o
FALLBACK_OBJECTS += src/tpm.o src/errlog.o

$(foreach x,$(SHIM_SOURCES) $(MOK_SOURCES) $(FALLBACK_SOURCES),$(eval vpath $(x) $(TOPDIR)))
$(foreach x,$(SHIM_OBJECTS) $(MOK_OBJECTS) $(FALLBACK_OBJECTS),$(eval vpath $(x) $(BUILDDIR)))
vpath %.so $(BUILDDIR)/src

SHIM_CLEAN_PREFIXES = $(MMNAME) $(FBNAME) $(SHIMNAME)
SHIM_CLEAN_SUFFIXES = .signed .debug .hash
SHIM_CLEAN_FILENAMES = $(foreach x,$(SHIM_CLEAN_PREFIXES),$(foreach y,$(SHIM_CLEAN_SUFFIXES),$(x)$(y)) $(x))
SHIM_CLEAN_FILES = $(foreach x,$(SHIM_CLEAN_FILENAMES),$(BUILDDIR)/$(x))
SHIM_CLEAN_FILES += $(BUILDDIR)/src/config.h $(BUILDDIR)/src/shim_cert.h
clean-shim-objs :
	@rm -f $(BUILDDIR)/src/*.{a,o,so}
	@rm -f $(SHIM_CLEAN_FILES)
	@rm -f $(foreach x,$(BOOTCSVNAME) $(BOOTEFINAME),$(BUILDDIR)/$(x))

clean : | clean-shim-objs

src/cert.o : src/cert.S
	$(CC) $(CFLAGS) -c -o $@ $<

src/shim.o: | src/shim.h
src/shim.h: | src/config.h

vpath src/config.h $(BUILDDIR)

.ONESHELL: src/config.h
src/config.h:
	@echo making config.h
	(
	echo "#define EFI_ARCH L\"$(ARCH)\""
	echo "#define DEBUGDIR L\"/usr/lib/debug/usr/share/shim/$(ARCH)-$(VERSION)$(DASHRELEASE)/\""
	echo "#define DEFAULT_LOADER L\"$(DEFAULT_LOADER)\""
	echo "#define DEFAULT_LOADER_CHAR \"$(DEFAULT_LOADER)\""
	echo $(CONFIG_VENDOR_CERT)
	echo $(CONFIG_VENDOR_DBX)
	echo $(CONFIG_ENABLE_SHIM_CERT)
	echo $(CONFIG_OVERRIDE_SECURITY_POLICY)
	echo $(CONFIG_ENABLE_HTTPBOOT)
	echo $(CONFIG_REQUIRE_TPM)
	) > $@

vpath version.c.in $(TOPDIR)/src/
vpath version.c $(BUILDDIR)/src/
vpath version.o $(BUILDDIR)/src/

src/version.o : src/version.c | version.c.in
src/version.c : version.c.in
	sed	-e "s,@@VERSION@@,$(VERSION)," \
		-e "s,@@UNAME@@,$(shell uname -s -m -p -i -o)," \
		-e "s,@@COMMIT@@,$(COMMIT_ID)," \
		< $< > $@

$(SHIMNAME) : $(SHIMSONAME)
$(MMNAME) : $(MMSONAME)
$(FBNAME) : $(FBSONAME)

SHIM_TARGETS = $(SHIMNAME) $(MMNAME) $(FBNAME)
TARGETS += $(foreach x,$(SHIMNAME) $(MMNAME) $(FBNAME),$(x) $(x).debug)

ifneq ($(origin ENABLE_SHIM_HASH),undefined)
TARGETS += $(SHIMHASHNAME)
endif
ifneq ($(origin ENABLE_SHIM_CERT),undefined)
.NOTPARALLEL: src/shim_cert.h
src/shim.h: | src/shim_cert.h
TARGETS	+= $(MMNAME).signed $(FBNAME).signed
endif

.ONESHELL: src/shim_cert.h
src/shim_cert.h: certdb/shim.cer
	@echo making shim_cert.h
	cd $(BUILDDIR)
	(
	echo "static UINT8 shim_cert[] __attribute__((__unused__)) = {"
	$(HEXDUMP) -v -e '1/1 "0x%02x, "' $<
	echo "};"
	) > $@

OPENSSL_GROUP=-Wl,--start-group,libcryptlib.a,libopenssl.a,--end-group

$(SHIMSONAME): | libcryptlib.a libopenssl.a src/shim_cert.h
$(SHIMSONAME): $(SHIM_OBJECTS) lib.a
	$(CC) -o $@ $(CFLAGS) $(CCLDFLAGS) $^ -lefi -lgnuefi $(OPENSSL_GROUP) $(LIB_GCC)

$(FBSONAME): $(FALLBACK_OBJECTS) lib.a
	$(CC) -o $@ $(CFLAGS) $(CCLDFLAGS) $^ -lefi -lgnuefi

$(MMSONAME): | libcryptlib.a libopenssl.a
$(MMSONAME): $(MOK_OBJECTS) lib.a
	$(CC) -o $@ $(CFLAGS) $(CCLDFLAGS) $^ -lefi -lgnuefi $(OPENSSL_GROUP)

vpath buildid $(BUILDDIR)/src
vpath buildid.c $(TOPDIR)/src
buildid : buildid.c
	$(CC) -Og -g3 -Wall -Werror -Wextra -o $(BUILDDIR)/$@ $< -lelf

$(BOOTCSVNAME) :
	@echo Making $@
	@echo "$(SHIMNAME),$(OSLABEL),,This is the boot entry for $(OSLABEL)" | iconv -t UCS-2LE > $@

install-deps : $(SHIMNAME).debug $(MMNAME).debug $(FBNAME).debug buildid
install-deps : $(BOOTCSVNAME)
