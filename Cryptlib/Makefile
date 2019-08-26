CRYPTLIB_SOURCES = $(shell cd $(TOPDIR) ; cat edk2/CryptoPkg/Library/BaseCryptLib/BaseCryptLib.inf | dos2unix | grep '.*\.c$$' | sed 's,^ \+,Cryptlib/,' | git check-ignore --stdin -n -v | grep :: | cut -d: -f3- | sort -u)

CRYPTLIB_INCLUDES = \
		    -I$(TOPDIR)/Cryptlib \
		    -I$(TOPDIR)/Cryptlib/Include \
		    -I$(TOPDIR)/Cryptlib/Include/openssl

CRYPTLIB_OBJECTS = $(foreach x,$(CRYPTLIB_SOURCES),$(patsubst %.c,%.o,$(x)))

$(foreach x,$(CRYPTLIB_SOURCES),$(eval vpath $(x) $(TOPDIR)))
$(foreach x,$(CRYPTLIB_OBJECTS),$(eval vpath $(x) $(BUILDDIR)))

$(foreach x,$(CRYPTLIB_SOURCES),$(eval $(call object-template,,$(x),$$(CRYPTLIB_INCLUDES))))

clean-cryptlib-objs :
	@rm -rf "$(BUILDDIR)/Cryptlib"

clean : | clean-cryptlib-objs

vpath libcryptlib.a $(BUILDDIR)
libcryptlib.a : $(CRYPTLIB_OBJECTS)
	$(AR) rcsD $@ $^
