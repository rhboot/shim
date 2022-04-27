TARGET = lib.a

LIBFILES_UNSORTED := $(patsubst %.c,%.o,$(subst $(TOPDIR)/lib/,,$(wildcard $(TOPDIR)/lib/*.c)))
LIBFILES := $(sort $(LIBFILES_UNSORTED))

CRYPTDIR	= $(TOPDIR)/Cryptlib

INCLUDES	= $(EFI_INCLUDES) \
		  -I$(TOPDIR)/include \
		  -I$(CRYPTDIR)/Include/openssl/ \
		  -I$(CRYPTDIR)/Include/ \
		  -I$(CRYPTDIR) \
		  -I$(TOPDIR) \
		  -isystem $(TOPDIR)/include/system \
		  -isystem $(shell $(CC) -print-file-name=include)

CLANG_BUGS	= $(if $(findstring gcc,$(CC)),-maccumulate-outgoing-args,)

ifeq ($(ARCH),x86_64)
FEATUREFLAGS	+= -m64 -mno-mmx -mno-sse -mno-red-zone -nostdinc $(CLANG_BUGS)
DEFINES		+= -DMDE_CPU_X64
endif
ifeq ($(ARCH),ia32)
FEATUREFLAGS	+= -m32 -mno-mmx -mno-sse -mno-red-zone -nostdinc $(CLANG_BUGS)
DEFINES		+= -DMDE_CPU_IA32
endif
ifeq ($(ARCH),aarch64)
DEFINES		+= -DMDE_CPU_AARCH64
endif
ifeq ($(ARCH),arm)
DEFINES		+= -DMDE_CPU_ARM
endif

LDFLAGS		= -nostdlib -znocombreloc


CFLAGS		= $(FEATUREFLAGS) \
		  $(OPTIMIZATIONS) \
		  $(WARNFLAGS) \
		  $(WERRFLAGS) \
		  $(INCLUDES) \
		  $(DEFINES)

ifneq ($(origin ENABLE_SHIM_DEVEL),undefined)
CFLAGS += -DENABLE_SHIM_DEVEL
endif

lib.a: $(LIBFILES)
	$(AR) rcs lib.a $(LIBFILES)

all: $(TARGET)

clean:
	rm -f lib.a
	rm -f $(LIBFILES)

