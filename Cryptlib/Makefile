
EFI_INCLUDES	= -IInclude -I$(EFI_INCLUDE) -I$(EFI_INCLUDE)/$(ARCH) -I$(EFI_INCLUDE)/protocol

CFLAGS		= -ggdb -O0 -I. -fno-stack-protector -fno-strict-aliasing -fpic -fshort-wchar \
		  -Wall $(EFI_INCLUDES) \
		  -ffreestanding -I$(shell $(CC) -print-file-name=include)

ifeq ($(ARCH),x86_64)
	CFLAGS	+= -mno-mmx -mno-sse -mno-red-zone -nostdinc -maccumulate-outgoing-args \
		-DEFI_FUNCTION_WRAPPER -DGNU_EFI_USE_MS_ABI
endif
ifeq ($(ARCH),ia32)
	CFLAGS	+= -mno-mmx -mno-sse -mno-red-zone -nostdinc -maccumulate-outgoing-args -m32
endif
LDFLAGS		= -nostdlib -znocombreloc

TARGET		= libcryptlib.a
OBJS		=   Hash/CryptMd4.o \
		    Hash/CryptMd5.o \
		    Hash/CryptSha1.o \
		    Hash/CryptSha256.o \
		    Hash/CryptSha512.o \
		    Hmac/CryptHmacMd5.o \
		    Hmac/CryptHmacSha1.o \
		    Cipher/CryptAes.o \
		    Cipher/CryptTdes.o \
		    Cipher/CryptArc4.o \
		    Rand/CryptRand.o \
		    Pk/CryptRsaBasic.o \
		    Pk/CryptRsaExtNull.o \
		    Pk/CryptPkcs7SignNull.o \
		    Pk/CryptPkcs7Verify.o \
		    Pk/CryptDhNull.o \
		    Pk/CryptTs.o \
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
