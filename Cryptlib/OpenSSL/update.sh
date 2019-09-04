#/bin/sh

set -eu

usage() {
    echo usage: ./update.sh DIRECTORY 1>&2
    exit 1
}

[[ $# -eq 1 ]] || usage
[[ -n "${1}" ]] || usage

DIR="${1}"

WORK_PATH="${PWD}"
OPENSSLLIB_PATH="${DIR}/CryptoPkg/Library/OpensslLib"
OPENSSL_PATH="${OPENSSLLIB_PATH}/openssl"

needcommit() {(
    pwd
    cd "${DIR}"
    if ! git diff-files --name-only -z | xargs -r -0 -n1 git add ; then
        pwd
        "${@}"
    fi
)}

cd "${OPENSSL_PATH}"

cd "${OPENSSLLIB_PATH}"
perl -I. -Iopenssl/ process_files.pl
set -x
cd openssl
pwd
needcommit git commit -a -m "Update OpenSSL"
cd ..
pwd
needcommit git add -A . openssl
needcommit git commit -m "Update OpenSSL" openssl

cd "${DIR}"
pwd
needcommit git commit -m "Update CryptoPkg and openssl" "${OPENSSL_PATH}" CryptoPkg
cd "${WORK_PATH}"

rsync -avSHP "${OPENSSLLIB_PATH}/buildinf.h" buildinf.h
rsync -avSHP "${OPENSSL_PATH}/e_os.h" e_os.h
git add buildinf.h e_os.h

mkdir -p crypto
rsync -avSHP --delete-during "${OPENSSL_PATH}"/crypto/ crypto/

git clean -f -d -X -- crypto/

git add crypto/

needcommit git commit -a -m "Update OpenSSL"
