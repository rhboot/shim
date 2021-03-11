COV_EMAIL=$(call get-config,coverity.email)
COV_TOKEN=$(call get-config,coverity.token)
COV_URL=$(call get-config,coverity.url)
COV_FILE=$(NAME)-coverity-$(VERSION)-$(COMMIT_ID).tar.bz2

include $(TOPDIR)/Make.rules

define prop
$(if $(findstring undefined,$(origin $(1))),,$(1)="$($1)")
endef

PROPOGATE_MAKE_FLAGS = ARCH ARCH_SUFFIX COLOR CC COMPILER CROSS_COMPILE

MAKEARGS = $(foreach x,$(PROPOGATE_MAKE_FLAGS),$(call prop,$(x)))

cov-clean :
	@rm -vf $(NAME)-coverity-*.tar.*
	@if [ -d cov-int ]; then rm -rf cov-int && echo "removed 'cov-int'"; fi

cov-file : | $(COV_FILE)

$(COV_FILE) : | cov-int
	tar caf $@ cov-int

cov-upload : | cov-file
	@if [ -n "$(COV_URL)" ] &&					\
	    [ -n "$(COV_TOKEN)" ] &&					\
	    [ -n "$(COV_EMAIL)" ] ;					\
	then								\
		echo curl --form token=$(COV_TOKEN) --form email="$(COV_EMAIL)" --form file=@"$(COV_FILE)" --form version=$(VERSION).1 --form description="$(COMMIT_ID)" "$(COV_URL)" ; \
		curl --form token=$(COV_TOKEN) --form email="$(COV_EMAIL)" --form file=@"$(COV_FILE)" --form version=$(VERSION).1 --form description="$(COMMIT_ID)" "$(COV_URL)" ; \
	else								\
		echo Coverity output is in $(COV_FILE) ;		\
	fi

cov-build-unchecked-cryptlib :  | clean-cryptlib-objs
cov-build-unchecked-cryptlib : Cryptlib/libcryptlib.a

cov-build-unchecked-openssl : | clean-openssl-objs
cov-build-unchecked-openssl : Cryptlib/OpenSSL/libopenssl.a

cov-build-all : CCACHE_DISABLE=1
cov-build-all : | clean clean-shim-objs clean-cryptlib-objs clean-openssl-objs
	+cov-build --dir cov-int $(MAKE) $(MAKEARGS) CCACHE_DISABLE=1 all

coverity-no-openssl : | cov-test
coverity-no-openssl : clean-shim-objs clean-cryptlib-objs cov-build-unchecked-openssl cov-build-all cov-file cov-upload

coverity-no-cryptlib : | cov-test
coverity-no-cryptlib : clean-shim-objs cov-build-unchecked-openssl cov-build-unchecked-cryptlib cov-build-all cov-file cov-upload

coverity : | cov-test
coverity : coverity-no-openssl cov-file cov-upload

coverity-all : | cov-test
coverity-all : clean cov-build-all cov-file cov-upload

clean : | cov-clean

COV_BUILD ?= $(shell x=$$(which --skip-alias --skip-functions cov-build 2>/dev/null) ; [ -n "$$x" ] && echo "$$x")

cov-test : ; $(if $(findstring /,$(COV_BUILD)),,$(error cov-build not found))

.PHONY : coverity cov-upload cov-clean cov-file cov-test
