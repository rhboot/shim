#
# Makefile
# Peter Jones, 2019-01-23 15:35
#
default: all

EFI_ARCH ?= $(shell $(CC) -dumpmachine | cut -f1 -d- | \
	    sed \
		-e s,aarch64,aa64, \
		-e 's,arm.*,arm,' \
		-e s,i[3456789]86,ia32, \
		-e s,x86_64,x64, \
		)

ifeq ($(MAKELEVEL),0)
TOPDIR != if [ "$$(git rev-parse --is-inside-work-tree)" = true ]; then echo ./$$(git rev-parse --show-cdup) ; else echo $(PWD) ; fi
BUILDDIR ?= $(TOPDIR)/build-$(EFI_ARCH)
endif

include $(TOPDIR)include/version.mk
export VERSION DASHRELEASE EFI_ARCH

all : | mkbuilddir
% : |
	@if ! [ -d $(BUILDDIR)/ ] ; then $(MAKE) BUILDDIR=$(BUILDDIR) TOPDIR=$(TOPDIR) mkbuilddir ; fi
	$(MAKE) TOPDIR=../$(TOPDIR) BUILDDIR=../$(BUILDDIR) -C $(BUILDDIR) -f Makefile $@

mkbuilddir :
	@mkdir -p $(BUILDDIR)
	@ln -f $(TOPDIR)/include/build.mk $(BUILDDIR)/Makefile

update :
	git submodule update --init --recursive
	cd $(TOPDIR)/edk2/ ; \
		git submodule set-branch --branch shim-$(VERSION) CryptoPkg/Library/OpensslLib/openssl
	cd $(TOPDIR) ; \
		git submodule set-branch --branch shim-$(VERSION) edk2 ; \
		git submodule sync --recursive
	cd $(TOPDIR)/edk2/CryptoPkg/Library/OpensslLib/openssl ; \
		git fetch origin ; \
		git checkout shim-$(VERSION) ; \
		git rebase origin/shim-$(VERSION)
	cd $(TOPDIR)/edk2/CryptoPkg/Library/OpensslLib ; \
		if ! git diff-index --quiet HEAD -- openssl ; then \
			git commit -m "Update openssl" openssl ; \
		fi
	cd $(TOPDIR)/edk2 ; : \
		git fetch origin ; \
		git checkout shim-$(VERSION) ; \
		git rebase origin/shim-$(VERSION)
	cd $(TOPDIR) ; \
		if ! git diff-index --quiet HEAD -- edk2 ; then \
			git commit -m "Update edk2" edk2 ; \
		fi
	cd $(TOPDIR)/Cryptlib ; \
		./update.sh

.PHONY: mkbuilddir update
.NOTPARALLEL:
# vim:ft=make
#
