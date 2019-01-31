#
# Makefile
# Peter Jones, 2019-01-23 15:35
#
default: all

include $(TOPDIR)/include/version.mk

ifeq ($(MAKELEVEL),0)
TOPDIR		?= ..
BUILDDIR	?= .
endif

include $(TOPDIR)include/version.mk
export VERSION DASHRELEASE

all : | mkbuilddir
% :
	$(MAKE) TOPDIR=../$(TOPDIR) BUILDDIR=../$(BUILDDIR) -C $(BUILDDIR) -f Makefile $@

mkbuilddir :
	@mkdir -p $(BUILDDIR)
	@ln -f $(TOPDIR)/include/build.mk $(BUILDDIR)/Makefile

.PHONY: mkbuilddir
# vim:ft=make
#
