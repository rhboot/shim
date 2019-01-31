CRYPTLIB_SOURCES = \
		   Cryptlib/Hash/CryptMd4Null.c \
		   Cryptlib/Hash/CryptMd5.c \
		   Cryptlib/Hash/CryptSha1.c \
		   Cryptlib/Hash/CryptSha256.c \
		   Cryptlib/Hash/CryptSha512.c \
		   Cryptlib/Hmac/CryptHmacMd5Null.c \
		   Cryptlib/Hmac/CryptHmacSha1Null.c \
		   Cryptlib/Hmac/CryptHmacSha256Null.c \
		   Cryptlib/Cipher/CryptAesNull.c \
		   Cryptlib/Cipher/CryptTdesNull.c \
		   Cryptlib/Cipher/CryptArc4Null.c \
		   Cryptlib/Rand/CryptRand.c \
		   Cryptlib/Pk/CryptRsaBasic.c \
		   Cryptlib/Pk/CryptRsaExtNull.c \
		   Cryptlib/Pk/CryptPkcs7SignNull.c \
		   Cryptlib/Pk/CryptPkcs7Verify.c \
		   Cryptlib/Pk/CryptDhNull.c \
		   Cryptlib/Pk/CryptTs.c \
		   Cryptlib/Pk/CryptX509.c \
		   Cryptlib/Pk/CryptAuthenticode.c \
		   Cryptlib/Pem/CryptPemNull.c \
		   Cryptlib/SysCall/CrtWrapper.c \
		   Cryptlib/SysCall/TimerWrapper.c \
		   Cryptlib/SysCall/BaseMemAllocation.c \
		   Cryptlib/SysCall/BaseStrings.c \
		   Cryptlib/SysCall/memset.c

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
