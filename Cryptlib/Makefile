
EFI_INCLUDES	= -I$(TOPDIR)/Include -I$(EFI_INCLUDE) -I$(EFI_INCLUDE)/$(ARCH) -I$(EFI_INCLUDE)/protocol

CFLAGS		= -ggdb -O0 -I$(TOPDIR) -iquote $(TOPDIR) -fno-stack-protector -fno-strict-aliasing -fpic -fshort-wchar \
		  -Wall $(EFI_INCLUDES) -std=gnu89 \
		  -ffreestanding -I$(shell $(CC) -print-file-name=include)

ifeq ($(ARCH),x86_64)
	CFLAGS	+= -mno-mmx -mno-sse -mno-red-zone -nostdinc -maccumulate-outgoing-args \
		-DEFI_FUNCTION_WRAPPER -DGNU_EFI_USE_MS_ABI -DNO_BUILTIN_VA_FUNCS \
		-DMDE_CPU_X64
endif
ifeq ($(ARCH),ia32)
	CFLAGS	+= -mno-mmx -mno-sse -mno-red-zone -nostdinc -maccumulate-outgoing-args -m32 \
		-DMDE_CPU_IA32
endif
ifeq ($(ARCH),aarch64)
	CFLAGS	+= -DMDE_CPU_AARCH64
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
		    Pem/CryptPem.o \
		    SysCall/CrtWrapper.o \
		    SysCall/TimerWrapper.o \
		    SysCall/BaseMemAllocation.o \
		    SysCall/BaseStrings.o \
		    SysCall/memset.o

all: $(TARGET)

libcryptlib.a: $(OBJS)
	ar rcs libcryptlib.a $(OBJS)
clean:
	rm -f $(TARGET) $(OBJS)
