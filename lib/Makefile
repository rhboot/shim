
VPATH=$(TOPDIR)/lib

TARGET = lib.a

LIBFILES = $(patsubst $(VPATH)/%.c,%.o,$(wildcard $(VPATH)/*.c))

EFI_INCLUDES += -I$(TOPDIR)/CryptLib/Include/openssl/

lib.a: $(LIBFILES)
	$(AR) rcs lib.a $(LIBFILES)

all: $(TARGET)

clean:
	rm -f lib.a
	rm -f $(LIBFILES)
