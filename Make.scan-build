SCAN_BUILD ?= $(shell x=$$(which --skip-alias --skip-functions scan-build 2>/dev/null) ; [ -n "$$x" ] && echo "$$x")

scan-test : ; $(if $(findstring /,$(SCAN_BUILD)),,$(error scan-build not found))

scan-clean :
	@if [[ -d scan-results ]]; then rm -rf scan-results && echo "removed 'scan-results'"; fi

scan-build : | scan-test
scan-build : clean-shim-objs
	make $(DASHJ) Cryptlib/OpenSSL/libopenssl.a Cryptlib/libcryptlib.a
	scan-build -o scan-results make $(DASHJ) CC=clang all

scan-build-all : | scan-test
scan-build-all : clean
	scan-build -o scan-results make $(DASHJ) CC=clang all

.PHONY : scan-build scan-clean
