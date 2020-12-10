
OPENSSLDIR = $(TOPDIR)/Cryptlib/OpenSSL
VPATH = $(TOPDIR)/Cryptlib

CRYPTLIB_CFLAGS = \
		  -isystem $(shell $(CC) $(ARCH_CFLAGS) -print-file-name=include) \
		  -I$(OPENSSLDIR)/crypto/asn1 \
		  -I$(OPENSSLDIR)/crypto/evp \
		  -I$(OPENSSLDIR)/crypto/modes \
		  -I$(OPENSSLDIR)/crypto/include \
		  -I$(OPENSSLDIR) \
		  -I$(TOPDIR)/Cryptlib/Include \
		  -iquote $(TOPDIR)/Cryptlib \
		  -iquote $(TOPDIR)/Cryptlib/Include \
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
       SysCall/BaseStrings.o

all: $(TARGET)

%.o : $(TOPDIR)/%.c

$(OBJS) :: %.o : %.c
	$(CC) $(CFLAGS) $(CRYPTLIB_CFLAGS) -c -o $@ $^

libcryptlib.a: $(OBJS)
	ar rcs libcryptlib.a $(OBJS)
clean:
	rm -f $(TARGET) $(OBJS)
