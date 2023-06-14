ifneq ($(CCACHE_DISABLE),)
export CCACHE_DISABLE
endif

CRYPTDIR	= $(TOPDIR)/Cryptlib

FEATUREFLAGS	+= -nostdinc

INCLUDES	= -I$(CRYPTDIR) -I$(CRYPTDIR)/Include \
		  $(EFI_INCLUDES) \
		  -isystem $(TOPDIR)/include/system \
		  -isystem $(shell $(CC) -print-file-name=include)

WARNFLAGS	+= -Wno-unused-parameter \
		   -Wno-unused-but-set-variable

CFLAGS		= $(FEATUREFLAGS) \
		  $(OPTIMIZATIONS) \
		  $(WARNFLAGS) \
		  $(WERRFLAGS) \
		  $(INCLUDES) \
		  $(DEFINES)

CLANG_BUGS	= $(if $(findstring gcc,$(CC)),-maccumulate-outgoing-args,)

ifeq ($(ARCH),x86_64)
FEATUREFLAGS	+= -m64 -mno-mmx -mno-sse -mno-red-zone $(CLANG_BUGS)
DEFINES		+= -DMDE_CPU_X64
endif
ifeq ($(ARCH),ia32)
FEATUREFLAGS	+= -m32 -mno-mmx -mno-sse -mno-red-zone $(CLANG_BUGS)
DEFINES		+= -DMDE_CPU_IA32
endif
ifeq ($(ARCH),aarch64)
DEFINES		+= -DMDE_CPU_AARCH64
endif
ifeq ($(ARCH),arm)
DEFINES		+= -DMDE_CPU_ARM
endif

LDFLAGS		= -nostdlib -znocombreloc

TARGET		= libcryptlib.a
OBJS		=   Hash/CryptMd4Null.o \
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

libcryptlib.a: $(OBJS)
	ar rcs libcryptlib.a $(OBJS)
clean:
	rm -f $(TARGET) $(OBJS)
