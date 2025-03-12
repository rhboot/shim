GCC_BINARY ?= $(shell x=$$(which --skip-alias --skip-functions gcc 2>/dev/null) ; [ -n "$$x" ] && echo "$$x")

fanalyzer-test : ; $(if $(findstring /,$(GCC_BINARY)),,$(error gcc not found))

define prop
$(if $(findstring undefined,$(origin $(1))),,$(eval export $(1)))
endef

PROPOGATE_MAKE_FLAGS = ARCH ARCH_SUFFIX COLOR CC COMPILER CROSS_COMPILE DASHJ

MAKEARGS = $(foreach x,$(PROPOGATE_MAKE_FLAGS),$(call prop,$(x)))

fanalyzer : | fanalyzer-test
fanalyzer : fanalyzer-no-openssl

fanalyzer-build-unchecked-cryptlib : Cryptlib/libcryptlib.a

fanalyzer-build-unchecked-openssl : Cryptlib/OpenSSL/libopenssl.a

fanalyzer-build-all : COMPILER=gcc
fanalyzer-build-all : CCACHE_DISABLE=1
fanalyzer-build-all : FEATUREFLAGS+=-fanalyzer
fanalyzer-build-all : WERRFLAGS=-Werror=analyzer-null-dereference
fanalyzer-build-all : IGNORE_COMPILER_ERRORS= || :
fanalyzer-build-all : all

fanalyzer-no-openssl : | fanalyzer-test
fanalyzer-no-openssl : clean-shim-objs clean-cryptlib-objs fanalyzer-build-unchecked-openssl fanalyzer-build-all

fanalyzer-no-cryptlib : | fanalyzer-test
fanalyzer-no-cryptlib : clean-shim-objs fanalyzer-build-unchecked-openssl fanalyzer-build-unchecked-cryptlib fanalyzer-build-all

fanalyzer-all : | fanalyzer-test
fanalyzer-all : clean fanalyzer-build-all

.PHONY : fanalyzer fanalyzer-build fanalyzer-all fanalyzer-build-all fanalyzer-clean
