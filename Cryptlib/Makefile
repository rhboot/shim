CRYPTLIB_SOURCES = $(shell cd $(TOPDIR) ; cat edk2/CryptoPkg/Library/BaseCryptLib/BaseCryptLib.inf | dos2unix | grep '.*\.c$$' | sed 's,^ \+,Cryptlib/,' | git check-ignore --stdin -n -v | grep :: | cut -d: -f3- | sort -u) \
	Cryptlib/Library/TimerLib.c Cryptlib/Library/Console.c

ifeq ($(EFI_ARCH),ia32)
	CRYPTLIB_SOURCES+=Cryptlib/Library/Ia32/GccInline.c
endif
ifeq ($(EFI_ARCH),x64)
	CRYPTLIB_SOURCES+=Cryptlib/Library/X64/GccInline.c
endif

CRYPTLIB_INCLUDES = \
		    -I$(TOPDIR)/Cryptlib \
		    -I$(TOPDIR)/Cryptlib/Include \
		    -I$(TOPDIR)/Cryptlib/Include/openssl

CRYPTLIB_OBJECTS = $(foreach x,$(CRYPTLIB_SOURCES),$(patsubst %.c,%.efi.o,$(x)))

$(foreach x,$(CRYPTLIB_SOURCES),$(eval vpath $(x) $(TOPDIR)))
$(foreach x,$(CRYPTLIB_OBJECTS),$(eval vpath $(x) $(BUILDDIR)))

$(foreach x,$(CRYPTLIB_SOURCES),$(eval $(call object-template,,$(x),$$(CRYPTLIB_INCLUDES) $$(OPENSSL_INCLUDES))))

clean-cryptlib-objs :
	@rm -rf "$(BUILDDIR)/Cryptlib"

clean : | clean-cryptlib-objs

vpath libcryptlib.efi.a $(BUILDDIR)
libcryptlib.efi.a : $(CRYPTLIB_OBJECTS)
	$(AR) rcsD $@ $^
