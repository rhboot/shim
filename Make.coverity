COV_EMAIL=$(call get-config,coverity.email)
COV_TOKEN=$(call get-config,coverity.token)
COV_URL=$(call get-config,coverity.url)
COV_FILE=$(NAME)-coverity-$(VERSION)-$(COMMIT_ID).tar.bz2

cov-int : clean-shim-objs
	make $(DASHJ) Cryptlib/OpenSSL/libopenssl.a Cryptlib/libcryptlib.a
	cov-build --dir cov-int make $(DASHJ) all

cov-int-all : clean
	cov-build --dir cov-int make $(DASHJ) all

cov-clean :
	@rm -vf $(NAME)-coverity-*.tar.*
	@if [[ -d cov-int ]]; then rm -rf cov-int && echo "removed 'cov-int'"; fi

cov-file : | $(COV_FILE)

$(COV_FILE) : | cov-int
	tar caf $@ cov-int

cov-upload :
	@if [[ -n "$(COV_URL)" ]] &&					\
	    [[ -n "$(COV_TOKEN)" ]] &&					\
	    [[ -n "$(COV_EMAIL)" ]] ;					\
	then								\
		echo curl --form token=$(COV_TOKEN) --form email="$(COV_EMAIL)" --form file=@"$(COV_FILE)" --form version=$(VERSION).1 --form description="$(COMMIT_ID)" "$(COV_URL)" ; \
		curl --form token=$(COV_TOKEN) --form email="$(COV_EMAIL)" --form file=@"$(COV_FILE)" --form version=$(VERSION).1 --form description="$(COMMIT_ID)" "$(COV_URL)" ; \
	else								\
		echo Coverity output is in $(COV_FILE) ;		\
	fi

coverity : | cov-test
coverity : cov-int cov-file cov-upload

coverity-all : | cov-test
coverity-all : cov-int-all cov-file cov-upload

clean : | cov-clean

COV_BUILD ?= $(shell x=$$(which --skip-alias --skip-functions cov-build 2>/dev/null) ; [ -n "$$x" ] && echo 1)
ifeq ($(COV_BUILD),)
	COV_BUILD_ERROR = $(error cov-build not found)
endif

cov-test : ; $(COV_BUILD_ERROR)

.PHONY : coverity cov-upload cov-clean cov-file cov-test
