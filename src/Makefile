
MMSTEM		?= mm$(EFI_ARCH_SUFFIX)
MMNAME		= $(MMSTEM).efi
MMSONAME	= $(MMSTEM).efi.so
FBSTEM		?= fb$(EFI_ARCH_SUFFIX)
FBNAME		= $(FBSTEM).efi
FBSONAME	= $(FBSTEM).efi.so
SHIMSTEM	?= shim$(EFI_ARCH_SUFFIX)
SHIMNAME	= $(SHIMSTEM).efi
SHIMSONAME	= $(SHIMSTEM).efi.so
SHIMHASHNAME	= $(SHIMSTEM).hash
BOOTEFINAME	?= BOOT$(ARCH_UPPER).EFI
BOOTCSVNAME	?= BOOT$(ARCH_UPPER).CSV

ORIG_SHIM_SOURCES = shim.c mok.c netboot.c replacements.c tpm.c errlog.c version.c httpboot.c
ORIG_MOK_SOURCES = MokManager.c PasswordCrypt.c crypt_blowfish.c
ORIG_FALLBACK_SOURCES = fallback.c

ifeq ($(SOURCE_DATE_EPOCH),)
	UNAME=$(shell uname -s -m -p -i -o)
else
	UNAME=buildhost
endif

SHIM_SOURCES := $(foreach x,$(ORIG_SHIM_SOURCES),src/$(x))
SHIM_OBJECTS := $(foreach x,$(SHIM_SOURCES),$(patsubst %.c,%.efi.o,$(x)))
MOK_SOURCES := $(foreach x,$(ORIG_MOK_SOURCES),src/$(x))
MOK_OBJECTS := $(foreach x,$(MOK_SOURCES),$(patsubst %.c,%.efi.o,$(x)))
FALLBACK_SOURCES := $(foreach x,$(ORIG_FALLBACK_SOURCES),src/$(x))
FALLBACK_OBJECTS := $(foreach x,$(FALLBACK_SOURCES),$(patsubst %.c,%.efi.o,$(x)))

$(foreach x,$(SHIM_SOURCES),$(eval $(call object-template,src/,$(x),$$(CRYPTLIB_INCLUDES))))
$(foreach x,$(MOK_SOURCES),$(eval $(call object-template,src/,$(x),$$(CRYPTLIB_INCLUDES))))
$(foreach x,$(FALLBACK_SOURCES),$(eval $(call object-template,src/,$(x),)))

SHIM_SOURCES += src/cert.S
SHIM_OBJECTS += src/cert.efi.o
FALLBACK_OBJECTS += src/tpm.efi.o src/errlog.efi.o

$(foreach x,$(SHIM_SOURCES) $(MOK_SOURCES) $(FALLBACK_SOURCES),$(eval vpath $(x) $(TOPDIR)))
$(foreach x,$(SHIM_OBJECTS) $(MOK_OBJECTS) $(FALLBACK_OBJECTS),$(eval vpath $(x) $(BUILDDIR)))
vpath %.efi.so $(BUILDDIR)/src

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

vpath src/config.h $(BUILDDIR)

EFI_BIN_SECTIONS += .data.ident .vendor_cert

.ONESHELL: src/config.h
src/config.h:
	@echo making config.h
	(
	echo "#define EFI_ARCH L\"$(EFI_ARCH)\""
	echo "#define DEBUGDIR L\"/usr/lib/debug/usr/share/shim/$(EFI_ARCH)-$(VERSION)$(DASHRELEASE)/\""
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
vpath version.efi.o $(BUILDDIR)/src/

src/version.efi.o : src/version.c | version.c.in
src/version.c : version.c.in
	@echo making version.c
	sed	-e "s,@@VERSION@@,$(VERSION)," \
		-e "s,@@UNAME@@,$(UNAME)," \
		-e "s,@@COMMIT@@,$(COMMIT_ID)," \
		-e "s,@@CONFIG@@,$(sort $(CONFIG_ITEMS))," \
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

# This gives us an ordering dep on everything in src/ and lib/ on the generated
# header files.
$(subst ../,,$(wildcard ../src/*.[chS] ../lib/*.[chS])) : | src/config.h src/shim_cert.h

$(SHIMSONAME): | lib.efi.a libcryptlib.efi.a libopenssl.efi.a
$(SHIMSONAME): EFI_LDLIBS+=lib.efi.a libcryptlib.efi.a libopenssl.efi.a
$(SHIMSONAME): $(SHIM_OBJECTS)

$(FBSONAME): | lib.efi.a
$(FBSONAME): EFI_LDLIBS+=lib.efi.a
$(FBSONAME): $(FALLBACK_OBJECTS)

$(MMSONAME): | lib.efi.a libcryptlib.efi.a libopenssl.efi.a
$(MMSONAME): EFI_LDLIBS+=lib.efi.a libcryptlib.efi.a libopenssl.efi.a
$(MMSONAME): $(MOK_OBJECTS)

vpath buildid $(BUILDDIR)/src
vpath buildid.c $(TOPDIR)/src
buildid : buildid.c
	$(CC) -Og -g3 -Wall -Werror -Wextra -o $(BUILDDIR)/$@ $< -lelf

$(BOOTCSVNAME) :
	@echo Making $@
	@echo "$(SHIMNAME),$(OSLABEL),,This is the boot entry for $(OSLABEL)" | iconv -t UCS-2LE > $@

install-deps : $(SHIMNAME).debug $(MMNAME).debug $(FBNAME).debug buildid
install-deps : $(BOOTCSVNAME)

setup:
	@git submodule update --init --recursive

.EXPORT_ALL_VARIABLES:
