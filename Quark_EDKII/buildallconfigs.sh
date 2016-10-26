#!/usr/bin/env bash
# This script is used to build all sets of config options

set -e

die()
{
    >&2 printf "$@"
    exit 1
}


if [ $# -eq 0 ]; then 
    die "%s [GCC4x] [Platform name]\n" "$0"
    exit 1
fi


# This is required when building with -DTPM_SUPPORT (or for EFI secure
# boot). Save users vain build time and a more cryptic (!)  error
# message
ossl_dir=CryptoPkg/Library/OpensslLib/openssl-0.9.8w

test -d "${ossl_dir}" ||
   die 'Missing OpenSSL dir %s
Please follow instructions in CryptoPkg/Library/OpensslLib/Patch-HOWTO.txt\n' "${ossl_dir}"


# Arrays require bash
FLAVOUR=([0]='PLAIN' [1]='SECURE')
ARG=(    [0]=''      [1]='-DSECURE_LD -DTPM_SUPPORT')


set -x
for num in 0 1; do
    ./quarkbuild.sh -d32 "$@" ${ARG[$num]}
    ./quarkbuild.sh -r32 "$@" ${ARG[$num]}
    (
        set -e
        cd Build/*Platform/
        mkdir  ${FLAVOUR[$num]}/
        ln -s RELEASE_GCC* RELEASE_GCC
        ln -s DEBUG_GCC* DEBUG_GCC
        mv RELEASE_GCC* DEBUG_GCC* ${FLAVOUR[$num]}/
    )
done

