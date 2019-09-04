#
# Makefile
# Peter Jones, 2019-01-23 15:35
#
default: all

ifeq ($(MAKELEVEL),0)
TOPDIR != if [ "$$(git rev-parse --is-inside-work-tree)" = true ]; then echo ./$$(git rev-parse --show-cdup) ; else echo .. ; fi
BUILDDIR ?= $(TOPDIR)/build-$(ARCH)
endif

include $(TOPDIR)include/version.mk
export VERSION DASHRELEASE

all : | mkbuilddir
% :
	$(MAKE) TOPDIR=../$(TOPDIR) BUILDDIR=../$(BUILDDIR) -C $(BUILDDIR) -f Makefile $@

mkbuilddir :
	@mkdir -p $(BUILDDIR)
	@ln -f $(TOPDIR)/include/build.mk $(BUILDDIR)/Makefile

update :
	cd $(TOPDIR)/edk2/ ; \
		git submodule set-branch --branch shim-$(VERSION) CryptoPkg/Library/OpensslLib/openssl
	cd $(TOPDIR) ; \
		git submodule set-branch --branch shim-$(VERSION) edk2 ; \
		git submodule sync --recursive
	cd $(TOPDIR)/edk2/CryptoPkg/Library/OpensslLib/openssl ; \
		git fetch rhboot ; \
		git rebase rhboot/shim-$(VERSION)
	cd $(TOPDIR)/edk2/CryptoPkg/Library/OpensslLib ; \
		if ! git diff-index --quiet HEAD -- openssl ; then \
			git commit -m "Update openssl" openssl ; \
		fi
	cd $(TOPDIR)/edk2 ; : \
		git fetch rhboot ; \
		git rebase rhboot/shim-$(VERSION)
	cd $(TOPDIR) ; \
		if ! git diff-index --quiet HEAD -- edk2 ; then \
			git commit -m "Update edk2" edk2 ; \
		fi
	cd $(TOPDIR)/Cryptlib ; \
		./update.sh

.PHONY: mkbuilddir update
# vim:ft=make
#
