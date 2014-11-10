TARGET = lib.a

LIBFILES = simple_file.o guid.o console.o execute.o configtable.o shell.o variables.o security_policy.o

EFI_INCLUDES    = -I$(EFI_INCLUDE) -I$(EFI_INCLUDE)/$(ARCH) -I$(EFI_INCLUDE)/protocol -I../include

CFLAGS          = -ggdb -O0 -fno-stack-protector -fno-strict-aliasing -fpic -nostdinc\
		  -fshort-wchar -Wall -DBUILD_EFI -fno-builtin -Werror \
		  $(EFI_INCLUDES)

ifeq ($(ARCH),x86_64)
        CFLAGS  += -mno-red-zone -DEFI_FUNCTION_WRAPPER -DGNU_EFI_USE_MS_ABI
endif
ifeq ($(ARCH),ia32)
        CFLAGS  += -mno-red-zone -m32
endif

lib.a: $(LIBFILES)
	ar rcs lib.a $(LIBFILES)

all: $(TARGET)

clean:
	rm -f lib.a
	rm -f $(LIBFILES)

