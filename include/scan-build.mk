SCAN_BUILD ?= $(shell x=$$(which --skip-alias --skip-functions scan-build 2>/dev/null) ; [ -n "$$x" ] && echo 1)
ifeq ($(SCAN_BUILD),)
	SCAN_BUILD_ERROR = $(error scan-build not found)
endif

scan-test : ; $(SCAN_BUILD_ERROR)

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
