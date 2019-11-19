default : all

NAME		= shim

ifeq ($(origin BUILDDIR),undefined)
	$(error BUILDDIR is not set)
endif
ifeq ($(origin TOPDIR),undefined)
	$(error TOPDIR is not set)
endif

include efi.mk

include $(TOPDIR)/include/config.mk
include $(TOPDIR)/include/defaults.mk
include $(TOPDIR)/include/coverity.mk
include $(TOPDIR)/include/scan-build.mk

include $(TOPDIR)/Cryptlib/Makefile
include $(TOPDIR)/Cryptlib/OpenSSL/Makefile
include $(TOPDIR)/src/Makefile
include $(TOPDIR)/lib/Makefile
include $(TOPDIR)/certdb/Makefile

SUBDIRS	= certdb lib src Cryptlib Cryptlib/OpenSSL

BUILDDIR_STEMS != find $(foreach x,$(SUBDIRS),$(TOPDIR)/$(x)) -type d
BUILDDIRS = $(foreach x,$(BUILDDIR_STEMS),$(subst $(TOPDIR)/,$(BUILDDIR)/,$(x)))

$(TARGETS) default all : | $(BUILDDIRS)

$(BUILDDIRS) : builddirs

builddirs:
	@mkdir -p $(BUILDDIRS)

$(foreach x,$(TARGETS),$(eval vpath $(x) $(BUILDDIR)))
$(foreach x,$(CRYPTLIB_SOURCES),$(eval vpath $(x) $(TOPDIR)/$(x)))
$(foreach x,$(CRYPTLIB_OBJECTS),$(eval vpath $(x) $(BUILDDIR)/$(x)))

all: $(TARGETS)

install-check :
ifeq ($(origin LIBDIR),undefined)
	$(error Architecture $(EFI_ARCH) is not a supported build target.)
endif
ifeq ($(origin EFIDIR),undefined)
	$(error EFIDIR must be set to your reserved EFI System Partition subdirectory name)
endif

install-deps : $(TARGETS)

install-debugsource : install-deps
	$(INSTALL) -d -m 0755 $(DESTDIR)/$(DEBUGSOURCE)/$(PKGNAME)-$(VERSION)$(DASHRELEASE)
	find $(BUILDDIR) -type f -a '(' -iname '*.c' -o -iname '*.h' -o -iname '*.S' ')' | while read file ; do \
		outfile=$$(echo $${file} | sed -e "s,^$(BUILDDIR),,") ; \
		$(INSTALL) -d -m 0755 $(DESTDIR)/$(DEBUGSOURCE)/$(PKGNAME)-$(VERSION)$(DASHRELEASE)/$$(dirname $${outfile}) ; \
		$(INSTALL) -m 0644 $${file} $(DESTDIR)/$(DEBUGSOURCE)/$(PKGNAME)-$(VERSION)$(DASHRELEASE)/$${outfile} ; \
	done

install-debuginfo : install-deps
	$(INSTALL) -d -m 0755 $(DESTDIR)/
	$(INSTALL) -d -m 0755 $(DESTDIR)/$(DEBUGINFO)$(TARGETDIR)/
	@./buildid $(wildcard *.efi.debug) | while read file buildid ; do \
		first=$$(echo $${buildid} | cut -b -2) ; \
		rest=$$(echo $${buildid} | cut -b 3-) ; \
		$(INSTALL) -d -m 0755 $(DESTDIR)/$(DEBUGINFO).build-id/$${first}/ ;\
		$(INSTALL) -m 0644 $${file} $(DESTDIR)/$(DEBUGINFO)$(TARGETDIR) ; \
		ln -s ../../../../..$(DEBUGINFO)$(TARGETDIR)$${file} $(DESTDIR)/$(DEBUGINFO).build-id/$${first}/$${rest}.debug ;\
		ln -s ../../../.build-id/$${first}/$${rest} $(DESTDIR)/$(DEBUGINFO).build-id/$${first}/$${rest} ;\
	done

install : | install-check
install : install-deps install-debuginfo install-debugsource
	$(INSTALL) -d -m 0755 $(DESTDIR)/
	$(INSTALL) -d -m 0700 $(DESTDIR)/$(ESPROOTDIR)
	$(INSTALL) -d -m 0755 $(DESTDIR)/$(EFIBOOTDIR)
	$(INSTALL) -d -m 0755 $(DESTDIR)/$(TARGETDIR)
	$(INSTALL) -m 0644 $(SHIMNAME) $(DESTDIR)/$(EFIBOOTDIR)/$(BOOTEFINAME)
	$(INSTALL) -m 0644 $(SHIMNAME) $(DESTDIR)/$(TARGETDIR)/
	$(INSTALL) -m 0644 $(BOOTCSVNAME) $(DESTDIR)/$(TARGETDIR)/
ifneq ($(origin ENABLE_SHIM_CERT),undefined)
	$(INSTALL) -m 0644 $(FBNAME).signed $(DESTDIR)/$(EFIBOOTDIR)/$(FBNAME)
	$(INSTALL) -m 0644 $(MMNAME).signed $(DESTDIR)/$(EFIBOOTDIR)/$(MMNAME)
	$(INSTALL) -m 0644 $(MMNAME).signed $(DESTDIR)/$(TARGETDIR)/$(MMNAME)
else
	$(INSTALL) -m 0644 $(FBNAME) $(DESTDIR)/$(EFIBOOTDIR)/
	$(INSTALL) -m 0644 $(MMNAME) $(DESTDIR)/$(EFIBOOTDIR)/
	$(INSTALL) -m 0644 $(MMNAME) $(DESTDIR)/$(TARGETDIR)/
endif

install-as-data : install-deps
	$(INSTALL) -d -m 0755 $(DESTDIR)/$(DATATARGETDIR)
	$(INSTALL) -m 0644 $(SHIMNAME) $(DESTDIR)/$(DATATARGETDIR)/
ifneq ($(origin ENABLE_SHIM_HASH),undefined)
	$(INSTALL) -m 0644 $(SHIMHASHNAME) $(DESTDIR)/$(DATATARGETDIR)/
endif
ifneq ($(origin ENABLE_SHIM_CERT),undefined)
	$(INSTALL) -m 0644 $(MMNAME).signed $(DESTDIR)/$(DATATARGETDIR)/$(MMNAME)
	$(INSTALL) -m 0644 $(FBNAME).signed $(DESTDIR)/$(DATATARGETDIR)/$(FBNAME)
else
	$(INSTALL) -m 0644 $(MMNAME) $(DESTDIR)/$(DATATARGETDIR)/$(MMNAME)
	$(INSTALL) -m 0644 $(FBNAME) $(DESTDIR)/$(DATATARGETDIR)/$(FBNAME)
endif

.ONESHELL: clean
clean :
	@rm -f $(TARGETS) $(BUILDDIR)/$(BOOTCSVNAME) $(BUILDDIR)/*.hash *.o *.a *.so
	@rm -f $(BUILDDIR)/src/version.c
	if [ "$(BUILDDIR)" != "$(TOPDIR)" ]; then
		find $(BUILDDIR)/ -depth -mindepth 1 -type d -exec rmdir {} \;
	fi

GITTAG = $(VERSION)

test-archive:
	@rm -rf /tmp/shim-$(VERSION) /tmp/shim-$(VERSION)-tmp
	@mkdir -p /tmp/shim-$(VERSION)-tmp
	@git archive --format=tar $(shell git branch | awk '/^*/ { print $$2 }') | ( cd /tmp/shim-$(VERSION)-tmp/ ; tar x )
	@git diff | ( cd /tmp/shim-$(VERSION)-tmp/ ; patch -s -p1 -b -z .gitdiff )
	@mv /tmp/shim-$(VERSION)-tmp/ /tmp/shim-$(VERSION)/
	@git log -1 --pretty=format:%H > /tmp/shim-$(VERSION)/commit
	@dir=$$PWD; cd /tmp; tar -c --bzip2 -f $$dir/shim-$(VERSION).tar.bz2 shim-$(VERSION)
	@rm -rf /tmp/shim-$(VERSION)
	@echo "The archive is in shim-$(VERSION).tar.bz2"

tag:
	git tag --sign $(GITTAG) refs/heads/master
	git tag -f latest-release $(GITTAG)

archive: tag
	@rm -rf /tmp/shim-$(VERSION) /tmp/shim-$(VERSION)-tmp
	@mkdir -p /tmp/shim-$(VERSION)-tmp
	@git archive --format=tar $(GITTAG) | ( cd /tmp/shim-$(VERSION)-tmp/ ; tar x )
	@mv /tmp/shim-$(VERSION)-tmp/ /tmp/shim-$(VERSION)/
	@git log -1 --pretty=format:%H > /tmp/shim-$(VERSION)/commit
	@dir=$$PWD; cd /tmp; tar -c --bzip2 -f $$dir/shim-$(VERSION).tar.bz2 shim-$(VERSION)
	@rm -rf /tmp/shim-$(VERSION)
	@echo "The archive is in shim-$(VERSION).tar.bz2"

.PHONY : install-deps all default
.EXPORT_ALL_VARIABLES:
unexport SUBDIRS
unexport BUILDDIR_STEMS
unexport BUILDDIRS
unexport TARGETS
