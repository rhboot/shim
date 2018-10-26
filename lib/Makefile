TARGET = lib.a

LIBFILES	= memory.o
ifeq ($(ARCH),x86_64)
	LIBFILES	= memory_x64.o
endif
ifeq ($(ARCH),ia32)
	LIBFILES	= memory_x86.o
endif

LIBFILES	+= simple_file.o guid.o console.o printf.o efilib.o dpath.o misc.o execute.o configtable.o shell.o variables.o security_policy.o

EFI_INCLUDES    = -I$(EFI_INCLUDE) -I$(EFI_INCLUDE)/$(ARCH_EFI) -I$(TOPDIR)/../include \
		  -I$(TOPDIR)/CryptLib/Include/openssl/

%.o : %.S
	$(CC) $(CFLAGS) -c -o $@ $<

lib.a: $(LIBFILES)
	$(AR) rcs lib.a $(LIBFILES)

all: $(TARGET)

clean:
	rm -f lib.a
	rm -f $(LIBFILES)

