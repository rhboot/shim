
OPENSSLDIR = $(TOPDIR)/Cryptlib/OpenSSL
VPATH = $(TOPDIR)/Cryptlib

CFLAGS		= -ggdb -O0 -I$(TOPDIR) -iquote $(TOPDIR) -fno-stack-protector -fno-strict-aliasing -fpic -fshort-wchar \
		  -Wall $(EFI_INCLUDES) -std=gnu89 \
		  -ffreestanding -I$(shell $(CC) -print-file-name=include)
CFLAGS += -I$(OPENSSLDIR)/crypto/asn1 \
	  -I$(OPENSSLDIR)/crypto/evp \
	  -I$(OPENSSLDIR)/crypto/modes \
	  -I$(OPENSSLDIR)/crypto/include \
	  -I$(OPENSSLDIR) \
	  -iquote $(OPENSSLDIR) \
	  -iquote $(OPENSSLDIR)/crypto/include

TARGET = libcryptlib.a
OBJS = Hash/CryptMd4Null.o \
       Hash/CryptMd5.o \
       Hash/CryptSha1.o \
       Hash/CryptSha256.o \
       Hash/CryptSha512.o \
       Hmac/CryptHmacMd5Null.o \
       Hmac/CryptHmacSha1Null.o \
       Hmac/CryptHmacSha256Null.o \
       Cipher/CryptAesNull.o \
       Cipher/CryptTdesNull.o \
       Cipher/CryptArc4Null.o \
       Rand/CryptRand.o \
       Pk/CryptRsaBasic.o \
       Pk/CryptRsaExtNull.o \
       Pk/CryptPkcs7SignNull.o \
       Pk/CryptPkcs7Verify.o \
       Pk/CryptDhNull.o \
       Pk/CryptTs.o \
       Pk/CryptX509.o \
       Pk/CryptAuthenticode.o \
       Pem/CryptPemNull.o \
       SysCall/CrtWrapper.o \
       SysCall/TimerWrapper.o \
       SysCall/BaseMemAllocation.o \
       SysCall/BaseStrings.o \
       SysCall/memset.o

%.o : $(TOPDIR)/%.c

all: $(TARGET)

libcryptlib.a: $(OBJS)
	ar rcs libcryptlib.a $(OBJS)
clean:
	rm -f $(TARGET) $(OBJS)
