include $(TOPDIR)/Cryptlib/cryptlib.mk

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
