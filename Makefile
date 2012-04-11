ARCH		= $(shell uname -m | sed s,i[3456789]86,ia32,)

LIB_PATH	= /usr/lib64

EFI_INCLUDE	= /usr/include/efi
EFI_INCLUDES	= -I$(EFI_INCLUDE) -I$(EFI_INCLUDE)/$(ARCH) -I$(EFI_INCLUDE)/protocol
EFI_PATH	= /usr/lib64/gnuefi

LIB_GCC		= $(shell $(CC) -print-libgcc-file-name)
EFI_LIBS	= -lefi -lgnuefi $(LIB_GCC)

EFI_CRT_OBJS 	= $(EFI_PATH)/crt0-efi-$(ARCH).o
EFI_LDS		= $(EFI_PATH)/elf_$(ARCH)_efi.lds


CFLAGS		= -O2 -fno-stack-protector -fno-strict-aliasing -fpic -fshort-wchar \
		  -Wall \
		  $(EFI_INCLUDES)
ifeq ($(ARCH),x86_64)
	CFLAGS	+= -DEFI_FUNCTION_WRAPPER
endif
LDFLAGS		= -nostdlib -znocombreloc -T $(EFI_LDS) -shared -Bsymbolic -L$(EFI_PATH) -L$(LIB_PATH) $(EFI_CRT_OBJS)

TARGET		= shim.efi
OBJS		= shim.o

all: $(TARGET)

shim.efi: $(OBJS)

%.efi: %.o 
	$(LD) $(LDFLAGS) $^ -o $@ $(EFI_LIBS)
	objcopy -j .text -j .sdata -j .data \
		-j .dynamic -j .dynsym  -j .rel \
		-j .rela -j .reloc \
		--target=efi-app-$(ARCH) $@
	strip $@

clean:
	rm -f $(TARGET) $(OBJS)
