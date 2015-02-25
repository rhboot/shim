TARGET = lib.a

LIBFILES = simple_file.o guid.o console.o execute.o configtable.o shell.o variables.o security_policy.o

EFI_INCLUDES    = -I$(EFI_INCLUDE) -I$(EFI_INCLUDE)/$(ARCH) -I$(EFI_INCLUDE)/protocol -I../include

lib.a: $(LIBFILES)
	ar rcs lib.a $(LIBFILES)

all: $(TARGET)

clean:
	rm -f lib.a
	rm -f $(LIBFILES)

