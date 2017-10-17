#/bin/sh
DIR=$1
OPENSSLLIB_PATH=$DIR/CryptoPkg/Library/OpensslLib
OPENSSL_PATH=$OPENSSLLIB_PATH/openssl

cp $OPENSSLLIB_PATH/buildinf.h buildinf.h
cp $OPENSSL_PATH/e_os.h e_os.h

mkdir -p crypto
C_FILES="
	LPdir_nyi.c
	cpt_err.c
	cryptlib.c
	cversion.c
	ebcdic.c
	ex_data.c
	init.c
	mem.c
	mem_clr.c
	mem_dbg.c
	mem_sec.c
	o_dir.c
	o_fips.c
	o_fopen.c
	o_init.c
	o_str.c
	o_time.c
	threads_none.c
	threads_pthread.c
	threads_win.c uid.c
"
for file in $C_FILES
do
	cp $OPENSSL_PATH/crypto/$file crypto
done

SUBDIRS="
	include/internal/
	aes
	asn1
	async/arch
	async
	bio
	bn
	buffer
	cmac
	comp
	conf
	dh
	dso
	err
	evp
	hmac
	kdf
	lhash
	md5
	modes
	objects
	ocsp
	pem
	pkcs12
	pkcs7
	rand
	rc4
	rsa
	sha
	stack
	txt_db
	x509
	x509v3
"
for dir in $SUBDIRS
do
	mkdir -p crypto/$dir
	cp $OPENSSL_PATH/crypto/$dir/*.[ch] crypto/$dir
done

# Remove unused files
rm -f crypto/aes/aes_x86core.c
rm -f crypto/x509v3/tabtest.c
rm -f crypto/x509v3/v3conf.c
rm -f crypto/x509v3/v3prin.c

find . -name "*.[ch]" -exec chmod -x {} \;

patch -p3 < openssl-bio-b_print-disable-sse.patch
patch -p3 < openssl-pk7-smime-error-message.patch
