LIB_SOURCES = $(foreach x,$(wildcard $(TOPDIR)/lib/*.c),$(subst $(TOPDIR)/,,$(x)))
LIB_OBJECTS = $(foreach x,$(LIB_SOURCES),$(patsubst %.c,%.efi.o,$(x)))

$(foreach x,$(LIB_SOURCES),$(eval vpath $(x) $(TOPDIR)))
$(foreach x,$(LIB_OBJECTS),$(eval vpath $(x) $(BUILDDIR)))
$(foreach x,$(LIB_SOURCES),$(eval $(call object-template,lib/,$(x),$$(CRYPTLIB_INCLUDES))))

clean-lib-objs :
	@rm -rf $(BUILDDIR)/lib

clean : | clean-lib-objs

vpath lib.efi.a $(BUILDDIR)/lib
lib.efi.a : $(LIB_OBJECTS)
	$(AR) rcsD $@ $^
