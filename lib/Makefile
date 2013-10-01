TARGET = lib.a

LIBFILES = simple_file.o guid.o console.o execute.o configtable.o shell.o variables.o security_policy.o

ARCH            = $(shell uname -m | sed s,i[3456789]86,ia32,)

EFI_INCLUDE     = /usr/include/efi
EFI_INCLUDES    = -I$(EFI_INCLUDE) -I$(EFI_INCLUDE)/$(ARCH) -I$(EFI_INCLUDE)/protocol -I../include

EFI_CRT_OBJS    = $(EFI_PATH)/crt0-efi-$(ARCH).o
EFI_LDS         = $(EFI_PATH)/elf_$(ARCH)_efi.lds

CFLAGS          = -ggdb -O0 -fno-stack-protector -fno-strict-aliasing -fpic \
		  -fshort-wchar -Wall -mno-red-zone -DBUILD_EFI -fno-builtin \
		  -Werror \
		  $(EFI_INCLUDES)
ifeq ($(ARCH),x86_64)
        CFLAGS  += -DEFI_FUNCTION_WRAPPER -DGNU_EFI_USE_MS_ABI
endif

lib.a: $(LIBFILES)
	ar rcs lib.a $(LIBFILES)

all: $(TARGET)

clean:
	rm -f lib.a
	rm -f $(LIBFILES)

