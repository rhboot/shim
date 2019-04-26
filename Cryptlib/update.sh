#!/usr/bin/env bash

set -eu

usage() {
    echo usage: ./update.sh 1>&2
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

CRYPTLIB="${PWD}"
TOPDIR="$(realpath "${CRYPTLIB}/..")"
BRANCH="$(git symbolic-ref --short HEAD)"

test_dir ../edk2/ || usage

cd "${DIR}"
git fetch
git rebase
git submodule update --recursive
cd ..

set -x
cd "${CRYPTLIB}/OpenSSL"
./update.sh "${DIR}"
cd "${CRYPTLIB}"

rsync -avSHP "${DIR}"/CryptoPkg/Library/BaseCryptLib/InternalCryptLib.h InternalCryptLib.h

for x in "${DIR}"/CryptoPkg/Library/BaseCryptLib/*/ ; do
    rsync -avSHP --delete-during "${x}" "${x##*BaseCryptLib/}"
done

rsync -avSHP "${DIR}"/CryptoPkg/Library/Include/CrtLibSupport.h Include/
rsync -avSHP "${DIR}"/CryptoPkg/Library/OpensslLib/openssl/include/openssl/ Include/openssl/
rsync -avSHP "${DIR}"/CryptoPkg/Library/OpensslLib/openssl/include/internal/ Include/internal/
rsync -avSHP "${DIR}"/CryptoPkg/Library/Include/internal/dso_conf.h Include/internal/

rsync -avSHP "${DIR}"/CryptoPkg/Library/Include/openssl/opensslconf.h Include/openssl/

git clean -f -d -X -- .
git add .

if ! git diff-index --quiet HEAD -- ${TOPDIR}/edk2 ; then
    git add ${TOPDIR}/edk2
fi

if ! git diff-index --quiet HEAD ; then
    pwd
    git commit -m "Update edk2, Cryptlib, and Cryptlib/OpenSSL"
fi
