TARGET = lib.a

LIBFILES = $(foreach x,$(wildcard *.c),$(patsubst %.c,%.o,$(x)))

EFI_INCLUDES    = -I$(EFI_INCLUDE) -I$(EFI_INCLUDE)/$(ARCH) -I$(EFI_INCLUDE)/protocol -I$(TOPDIR)/../include \
		  -I$(TOPDIR)/CryptLib/Include/openssl/

lib.a: $(LIBFILES)
	$(AR) rcs lib.a $(LIBFILES)

all: $(TARGET)

clean:
	rm -f lib.a
	rm -f $(LIBFILES)

