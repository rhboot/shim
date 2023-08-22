SCAN_BUILD ?= $(shell x=$$(which --skip-alias --skip-functions scan-build 2>/dev/null) ; [ -n "$$x" ] && echo "$$x")

scan-test : ; $(if $(findstring /,$(SCAN_BUILD)),,$(error scan-build not found))

define prop
$(if $(findstring undefined,$(origin $(1))),,$(1)="$($1)")
endef

PROPOGATE_MAKE_FLAGS = ARCH ARCH_SUFFIX COLOR CC COMPILER CROSS_COMPILE DASHJ

MAKEARGS = $(foreach x,$(PROPOGATE_MAKE_FLAGS),$(call prop,$(x)))

scan-clean :
	@if [[ -d scan-results ]]; then rm -rf scan-results && echo "removed 'scan-results'"; fi

scan : | scan-test
scan : clean-shim-objs clean-cryptlib-objs scan-build-no-openssl

scan-build-unchecked-cryptlib : Cryptlib/libcryptlib.a

scan-build-unchecked-openssl : Cryptlib/OpenSSL/libopenssl.a

scan-build-all : CCACHE_DISABLE=1
scan-build-all : COMPILER=clang
scan-build-all : IGNORE_COMPILER_ERRORS=" || :"
scan-build-all : | scan-test
scan-build-all :
	+scan-build -o scan-results make $(MAKEARGS) $(DASHJ) CCACHE_DISABLE=1 all

scan-build-no-openssl : | scan-test
scan-build-no-openssl : clean-shim-objs clean-cryptlib-objs scan-build-unchecked-openssl scan-build-all

scan-build-no-cryptlib : | scan-test
scan-build-no-cryptlib : clean-shim-objs scan-build-unchecked-cryptlib scan-build-unchecked-openssl scan-build-all

scan-all : | scan-test
scan-all : clean scan-build-all

.PHONY : scan-build scan-clean
