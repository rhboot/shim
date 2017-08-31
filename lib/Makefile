TARGET = lib.a

LIBFILES = simple_file.o guid.o console.o execute.o configtable.o shell.o variables.o security_policy.o

EFI_INCLUDES    = -I$(EFI_INCLUDE) -I$(EFI_INCLUDE)/$(ARCH) -I$(EFI_INCLUDE)/protocol -I$(TOPDIR)/../include \
		  -I$(TOPDIR)/CryptLib/Include/openssl/

lib.a: $(LIBFILES)
	$(AR) rcs lib.a $(LIBFILES)

all: $(TARGET)

clean:
	rm -f lib.a
	rm -f $(LIBFILES)

