OPENSSL_CRYPTO_SOURCES=$(shell cat $(TOPDIR)/edk2/CryptoPkg/Library/OpensslLib/OpensslLibCrypto.inf | dos2unix | grep '$(OPENSSL_PATH)/.*\.c$$' | cut -d/ -f2-)
OPENSSL_SOURCES = $(shell cd $(TOPDIR) ; ( $(foreach x,$(OPENSSL_CRYPTO_SOURCES),echo Cryptlib/OpenSSL/${x} ; ) :; ) | git check-ignore --stdin -n -v  | grep :: | cut -d: -f3- | sort -u)

OPENSSL_OBJECTS = $(foreach x,$(OPENSSL_SOURCES),$(patsubst %.c,%.efi.o,$(x)))
OPENSSL_CPPFLAGS += \
		    -Wno-unused-but-set-variable \
		    -DOPENSSL_SMALL_FOOTPRINT \
		    -DOPENSSL_FIPS \
		    -DOPENSSL_UEFI_APP \
		    -DOPENSSL_NO_DEPRECATED \
		    -DL_ENDIAN \
		    -DPEDANTIC \
		    -DDSO_NONE

OPENSSL_INCLUDES = \
		   -I$(OPENSSLDIR) \
		   -I$(OPENSSLDIR)/crypto/asn1 \
		   -I$(OPENSSLDIR)/crypto/evp \
		   -I$(OPENSSLDIR)/crypto/modes \
		   -I$(OPENSSLDIR)/crypto/include

$(foreach x,$(OPENSSL_SOURCES),$(eval vpath $(x) $(TOPDIR)))
$(foreach x,$(OPENSSL_OBJECTS),$(eval vpath $(x) $(BUILDDIR)))

$(foreach x,$(OPENSSL_SOURCES),$(eval $(call object-template,,$(x),$$(CRYPTLIB_INCLUDES) $$(OPENSSL_INCLUDES))))

clean : |

vpath libopenssl.efi.a $(BUILDDIR)
libopenssl.efi.a : $(OPENSSL_OBJECTS)
	$(AR) rcsD $@ $^
