#
# Makefile
# Peter Jones, 2019-01-17 15:44
#

vpath %.crt $(BUILDDIR)/certdb
vpath %.cer $(BUILDDIR)/certdb
vpath %.key $(BUILDDIR)/certdb
vpath %.p12 $(BUILDDIR)/certdb

.ONESHELL: certdb/secmod.db
certdb/secmod.db: certdb/shim.crt certdb/shim.p12
	@cd $(BUILDDIR)/certdb
	$(PK12UTIL) -d ./ -i ./shim.p12 -W "" -K "" >/dev/null 2>&1
	$(CERTUTIL) -d ./ -A -i ./shim.crt -n ./ -t u >/dev/null 2>&1

.ONESHELL: certdb/shim.crt certdb/shim.p12
certdb/shim.crt certdb/shim.p12:
	@cd $(BUILDDIR)/certdb
	echo making shim.crt
	$(TOPDIR)/../certdb/make-certs shim shim@xn--u4h.net all codesign 1.3.6.1.4.1.311.10.3.1 </dev/null >/dev/null

.ONESHELL: certdb/shim.cer
certdb/shim.cer: certdb/shim.crt
	@cd $(BUILDDIR)
	echo making shim.cer
	$(OPENSSL) x509 -outform der -in $< -out $@ >/dev/null

clean-certdb :
	@rm -rf $(BUILDDIR)/certdb
	@rm -f $(TOPDIR)/certdb/*.{cer,crl,crt,csr,db,domain.txt,old,p12,pem}

clean : | clean-certdb

# vim:ft=make
#
