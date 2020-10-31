LIB_SOURCES = $(foreach x,$(wildcard $(TOPDIR)/lib/*.c),$(subst $(TOPDIR)/,,$(x)))
ifeq ($(EFI_ARCH),x64)
	LIB_SOURCES	:= $(filter-out lib/memory.c,$(LIB_SOURCES)) lib/memory_x64.S
endif
ifeq ($(EFI_ARCH),ia32)
	LIB_SOURCES := $(filter-out lib/memory.c,$(LIB_SOURCES)) lib/memory_x86.S
endif

LIB_OBJECTS = $(foreach x,$(LIB_SOURCES),$(patsubst %.S,%.efi.o,$(patsubst %.c,%.efi.o,$(x))))

$(foreach x,$(LIB_SOURCES),$(eval vpath $(x) $(TOPDIR)))
$(foreach x,$(LIB_OBJECTS),$(eval vpath $(x) $(BUILDDIR)))
$(foreach x,$(LIB_SOURCES),$(eval $(call object-template,lib/,$(x),$$(CRYPTLIB_INCLUDES))))

clean-lib-objs :
	@rm -rf $(BUILDDIR)/lib

clean : | clean-lib-objs

vpath lib.efi.a $(BUILDDIR)/lib
lib.efi.a : $(LIB_OBJECTS)
	$(AR) rcsD $@ $^
