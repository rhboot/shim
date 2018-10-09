#!/bin/bash

set -eu

usage() {
    echo usage: ./update.sh DIRECTORY 1>&2
    exit 1
}

test_dir() {
    if [[ -d "${1}/CryptoPkg/Library/BaseCryptLib/" ]] && \
       [[ -d "${1}/CryptoPkg/Library/Include/internal/" ]] && \
       [[ -d "${1}/CryptoPkg/Library/OpensslLib/openssl/" ]] ; then
        DIR="$(realpath "${1}")"
        return 0
    fi
    return 1
}

TAG=$(mktemp -p shim -t -u 'shim-tag-XXXXXXXXXX' | cut -d/ -f2)
git tag "${TAG}"

git list --topo-order --reverse openssl-rebase-helper-start^..openssl-rebase-helper-end | cut -d\  -f1 | xargs git cherry-pick

if [[ $# -eq 1 ]] ; then
    test_dir "${1}" || usage
else
    test_dir .. || usage
fi

cd OpenSSL
./update.sh $DIR
cd ..

cp $DIR/CryptoPkg/Library/BaseCryptLib/InternalCryptLib.h InternalCryptLib.h
cp $DIR/CryptoPkg/Library/BaseCryptLib/Hash/CryptMd4Null.c Hash/CryptMd4Null.c
cp $DIR/CryptoPkg/Library/BaseCryptLib/Hash/CryptMd5.c Hash/CryptMd5.c
cp $DIR/CryptoPkg/Library/BaseCryptLib/Hash/CryptSha1.c Hash/CryptSha1.c
cp $DIR/CryptoPkg/Library/BaseCryptLib/Hash/CryptSha256.c Hash/CryptSha256.c
cp $DIR/CryptoPkg/Library/BaseCryptLib/Hash/CryptSha512.c Hash/CryptSha512.c
cp $DIR/CryptoPkg/Library/BaseCryptLib/Hmac/CryptHmacMd5Null.c Hmac/CryptHmacMd5Null.c
cp $DIR/CryptoPkg/Library/BaseCryptLib/Hmac/CryptHmacSha1Null.c Hmac/CryptHmacSha1Null.c
cp $DIR/CryptoPkg/Library/BaseCryptLib/Hmac/CryptHmacSha256Null.c Hmac/CryptHmacSha256Null.c
cp $DIR/CryptoPkg/Library/BaseCryptLib/Cipher/CryptAesNull.c Cipher/CryptAesNull.c
cp $DIR/CryptoPkg/Library/BaseCryptLib/Cipher/CryptTdesNull.c Cipher/CryptTdesNull.c
cp $DIR/CryptoPkg/Library/BaseCryptLib/Cipher/CryptArc4Null.c Cipher/CryptArc4Null.c
cp $DIR/CryptoPkg/Library/BaseCryptLib/Rand/CryptRand.c Rand/CryptRand.c
cp $DIR/CryptoPkg/Library/BaseCryptLib/Pk/CryptRsaBasic.c Pk/CryptRsaBasic.c
cp $DIR/CryptoPkg/Library/BaseCryptLib/Pk/CryptRsaExtNull.c Pk/CryptRsaExtNull.c
cp $DIR/CryptoPkg/Library/BaseCryptLib/Pk/CryptPkcs7SignNull.c Pk/CryptPkcs7SignNull.c
cp $DIR/CryptoPkg/Library/BaseCryptLib/Pk/CryptPkcs7Verify.c Pk/CryptPkcs7Verify.c
cp $DIR/CryptoPkg/Library/BaseCryptLib/Pk/CryptDhNull.c Pk/CryptDhNull.c
cp $DIR/CryptoPkg/Library/BaseCryptLib/Pk/CryptTs.c Pk/CryptTs.c
cp $DIR/CryptoPkg/Library/BaseCryptLib/Pk/CryptX509.c Pk/CryptX509.c
cp $DIR/CryptoPkg/Library/BaseCryptLib/Pk/CryptAuthenticode.c Pk/CryptAuthenticode.c
cp $DIR/CryptoPkg/Library/BaseCryptLib/Pem/CryptPemNull.c Pem/CryptPemNull.c
cp $DIR/CryptoPkg/Library/BaseCryptLib/SysCall/CrtWrapper.c SysCall/CrtWrapper.c
cp $DIR/CryptoPkg/Library/BaseCryptLib/SysCall/TimerWrapper.c SysCall/TimerWrapper.c
cp $DIR/CryptoPkg/Library/BaseCryptLib/SysCall/BaseMemAllocation.c SysCall/BaseMemAllocation.c

cp $DIR/CryptoPkg/Library/OpensslLib/openssl/include/openssl/*.h Include/openssl/
cp $DIR/CryptoPkg/Library/OpensslLib/openssl/include/internal/*.h Include/internal/
cp $DIR/CryptoPkg/Library/Include/internal/dso_conf.h Include/internal/

cp $DIR/CryptoPkg/Library/Include/openssl/opensslconf.h Include/openssl/

git add -A .
git commit -m "Update Cryptlib"

git config --local --add am.keepcr true
git am \
    0001-Cryptlib-update-for-efi-build.patch \
    0002-Cryptlib-work-around-new-CA-rules.patch \
    0003-Cryptlib-Pk-CryptX509.c-Fix-RETURN_-to-be-EFI_.patch

cd $DIR
git rm -r CryptoPkg
git commit -m "Get rid of the vestigial CryptoPkg"
git reset "${TAG}"
git add -A Cryptlib/
git commit -m "Update Cryptlib and OpenSSL"
git tag -d "${TAG}"
