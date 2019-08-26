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

update : TOPDIR=$(shell pwd)
update :
	cd $(TOPDIR) ; \
		git submodule sync --recursive
	cd $(TOPDIR)/edk2/CryptoPkg/Library/OpensslLib/openssl ; \
		git fetch rhboot ; \
		git rebase rhboot/shim
	cd $(TOPDIR)/edk2/CryptoPkg/Library/OpensslLib ; \
		if ! git diff-index --quiet HEAD -- openssl ; then \
			git commit -m "Update openssl" openssl ; \
		fi
	cd $(TOPDIR)/edk2 ; git fetch rhboot ; git rebase rhboot/shim
	cd $(TOPDIR) ; \
		if ! git diff-index --quiet HEAD -- edk2 ; then \
			git commit -m "Update edk2" edk2 ; \
		fi
	cd $(TOPDIR)/Cryptlib ; \
		./update.sh

.PHONY: mkbuilddir update
# vim:ft=make
#
