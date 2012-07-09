ARCH		= $(shell uname -m | sed s,i[3456789]86,ia32,)

SUBDIRS		= Cryptlib

LIB_PATH	= /usr/lib64

EFI_INCLUDE	= /usr/include/efi
EFI_INCLUDES	= -nostdinc -ICryptlib -ICryptlib/Include -I$(EFI_INCLUDE) -I$(EFI_INCLUDE)/$(ARCH) -I$(EFI_INCLUDE)/protocol 
EFI_PATH	= /usr/lib64/gnuefi

LIB_GCC		= $(shell $(CC) -print-libgcc-file-name)
EFI_LIBS	= -lefi -lgnuefi --start-group Cryptlib/libcryptlib.a Cryptlib/OpenSSL/libopenssl.a --end-group $(LIB_GCC) 

EFI_CRT_OBJS 	= $(EFI_PATH)/crt0-efi-$(ARCH).o
EFI_LDS		= $(EFI_PATH)/elf_$(ARCH)_efi.lds


CFLAGS		= -ggdb -O0 -fno-stack-protector -fno-strict-aliasing -fpic -fshort-wchar \
		  -Wall -mno-red-zone \
		  $(EFI_INCLUDES)
ifeq ($(ARCH),x86_64)
	CFLAGS	+= -DEFI_FUNCTION_WRAPPER
endif
LDFLAGS		= -nostdlib -znocombreloc -T $(EFI_LDS) -shared -Bsymbolic -L$(EFI_PATH) -L$(LIB_PATH) -LCryptlib -LCryptlib/OpenSSL $(EFI_CRT_OBJS)

TARGET		= shim.efi
OBJS		= shim.o shim.so
SOURCES		= shim.c shim.h signature.h PeImage.h cert.h

all: $(TARGET)

shim.o: $(SOURCES)

shim.so: $(OBJS) Cryptlib/libcryptlib.a Cryptlib/OpenSSL/libopenssl.a
	$(LD) -o $@ $(LDFLAGS) $^ $(EFI_LIBS)

Cryptlib/libcryptlib.a:
	$(MAKE) -C Cryptlib

Cryptlib/OpenSSL/libopenssl.a:
	$(MAKE) -C Cryptlib/OpenSSL

%.efi: %.so
	objcopy -j .text -j .sdata -j .data \
		-j .dynamic -j .dynsym  -j .rel \
		-j .rela -j .reloc -j .eh_frame \
		--target=efi-app-$(ARCH) $^ $@
	objcopy -j .text -j .sdata -j .data \
		-j .dynamic -j .dynsym  -j .rel \
		-j .rela -j .reloc -j .eh_frame \
		-j .debug_info -j .debug_abbrev -j .debug_aranges \
		-j .debug_line -j .debug_str -j .debug_ranges \
		--target=efi-app-$(ARCH) $^ shim.efi.debug

clean:
	$(MAKE) -C Cryptlib clean
	$(MAKE) -C Cryptlib/OpenSSL clean
	rm -f $(TARGET) $(OBJS)
