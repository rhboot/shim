ARCH		= $(shell uname -m | sed s,i[3456789]86,ia32,)

LIB_PATH	= /usr/lib64

EFI_INCLUDE	= /usr/include/efi
EFI_INCLUDES	= -nostdinc -IInclude -I$(EFI_INCLUDE) -I$(EFI_INCLUDE)/$(ARCH) -I$(EFI_INCLUDE)/protocol
EFI_PATH	= /usr/lib64/gnuefi

LIB_GCC		= $(shell $(CC) -print-libgcc-file-name)
EFI_LIBS	= -lefi -lgnuefi $(LIB_GCC)

CFLAGS		= -ggdb -O0 -I. -fno-stack-protector -fno-strict-aliasing -fpic -fshort-wchar \
		  -Wall $(EFI_INCLUDES) -mno-red-zone -maccumulate-outgoing-args -mno-sse -mno-mmx
ifeq ($(ARCH),x86_64)
	CFLAGS	+= -DEFI_FUNCTION_WRAPPER -DGNU_EFI_USE_MS_ABI
endif
LDFLAGS		= -nostdlib -znocombreloc

TARGET		= libcryptlib.a
OBJS		=   Hash/CryptMd4.o \
		    Hash/CryptMd5.o \
		    Hash/CryptSha1.o \
		    Hash/CryptSha256.o \
		    Hmac/CryptHmacMd5.o \
		    Hmac/CryptHmacSha1.o \
		    Cipher/CryptAes.o \
		    Cipher/CryptTdes.o \
		    Cipher/CryptArc4.o \
		    Rand/CryptRand.o \
		    Pk/CryptRsaBasic.o \
		    Pk/CryptRsaExt.o \
		    Pk/CryptPkcs7.o \
		    Pk/CryptDh.o \
		    Pk/CryptX509.o \
		    Pk/CryptAuthenticode.o \
		    Pem/CryptPem.o \
		    SysCall/CrtWrapper.o \
		    SysCall/TimerWrapper.o \
		    SysCall/BaseMemAllocation.o \
		    SysCall/BaseStrings.o

all: $(TARGET)

libcryptlib.a: $(OBJS)
	ar rcs libcryptlib.a $(OBJS)
clean:
	rm -f $(TARGET) $(OBJS)
