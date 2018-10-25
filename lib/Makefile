
VPATH=$(TOPDIR)/lib

TARGET = lib.a

LIBFILES = $(patsubst $(VPATH)/%.c,%.o,$(wildcard $(VPATH)/*.c))

LIB_CFLAGS = \
	     -I$(TOPDIR)/include \
	     -I$(TOPDIR)/Cryptlib \
	     -I$(TOPDIR)/Cryptlib/Include/ \
	     -iquote $(TOPDIR)

lib.a: $(LIBFILES)
	$(AR) rcs lib.a $(LIBFILES)

all: $(TARGET)

$(LIBFILES) :: %.o : %.c
	$(CC) $(CFLAGS) $(LIB_CFLAGS) -c -o $@ $^

clean:
	rm -f lib.a
	rm -f $(LIBFILES)
