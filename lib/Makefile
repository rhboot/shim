# include $(TOPDIR)/Make.defaults

TARGET = lib.a
LIBFILES = $(patsubst %.c,%.o,$(wildcard *.c))
INCLUDES += -I$(TOPDIR)/include/

CFLAGS = -std=gnu89 -ggdb -nostdinc \
	 $(CONFIGFLAGS) \
	 $(OPTIMIZATIONS) \
	 $(FEATURE_FLAGS) \
	 $(WARNINGS) \
	 $(ARCH_CFLAGS) \
	 $(INCLUDES) \
	 $(DEFINES)

lib.a: $(LIBFILES)
	$(AR) rcs lib.a $(LIBFILES)

all: $(TARGET)

clean:
	rm -f lib.a
	rm -f $(LIBFILES)

