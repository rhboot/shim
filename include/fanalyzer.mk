GCC_BINARY ?= $(shell x=$$(which --skip-alias --skip-functions gcc 2>/dev/null) ; [ -n "$$x" ] && echo "$$x")

fanalyzer-test : ; $(if $(findstring /,$(GCC_BINARY)),,$(error gcc not found))

define prop
$(if $(filter-out undefined,$(origin $(1))),$(1)=$($1),)
endef

MAKEARGS := \
	    $(call prop,ARCH) \
	    $(call prop,COLOR) \
	    $(call prop,CROSS_COMPILE)

fanalyzer : | fanalyzer-test
fanalyzer : clean-shim-objs fanalyzer-build

fanalyzer-build :
	make CC=gcc $(MAKEARGS) $(DASHJ) Cryptlib/OpenSSL/libopenssl.a Cryptlib/libcryptlib.a
	make CC=gcc $(MAKEARGS) FANALYZER=true all

fanalyzer-all : | fanalyzer-test
fanalyzer-all : clean fanalyzer-build-all

fanalyzer-build-all :
	make CC=gcc $(MAKEARGS) FANALYZER=true all

.PHONY : fanalyzer fanalyzer-build fanalyzer-all fanalyzer-build-all fanalyzer-clean
