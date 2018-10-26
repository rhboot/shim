
EFI_INCLUDES	= -I$(TOPDIR)/Include -I$(EFI_INCLUDE) -I$(EFI_INCLUDE)/$(ARCH_EFI) -I$(TOPDIR)/../include

CFLAGS		= $(DEBUG_CFLAGS)	\
			  -I$(TOPDIR) -iquote $(TOPDIR) -Wall -fshort-wchar	\
			  -fno-stack-protector -fno-strict-aliasing $(EFI_INCLUDES)	\
			  -std=gnu89 -UWIN32 -U_WIN32 -U__CYGWIN_ -ffunction-sections	\
			  -fdata-sections -ffreestanding -integrated-as \
			  -I$(shell $(CC) -print-file-name=include)

CLANG_BUGS	= $(if $(findstring gcc,$(CC)),-maccumulate-outgoing-args,)

ifeq ($(ARCH),x86_64)
	CFLAGS	+= -mno-red-zone -nostdinc $(CLANG_BUGS)	\
		   -m64 -mno-stack-arg-probe -target x86_64-windows-gnu
endif
ifeq ($(ARCH),ia32)
	CFLAGS	+= -mno-mmx -mno-sse -mno-red-zone -nostdinc \
		   $(CLANG_BUGS) -m32 -mregparm=3 -mrtd	\
		   -mno-stack-arg-probe -march=i586 -target i586-windows-gnu
endif
ifeq ($(ARCH),aarch64)
	CFLAGS	+= -mno-stack-arg-probe -target aarch64-windows-gnu
endif
ifeq ($(ARCH),arm)
	CFLAGS	+= -mno-stack-arg-probe -target arm-windows-gnu
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
