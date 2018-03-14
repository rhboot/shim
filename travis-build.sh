#!/bin/bash
#
# travis-build.sh
# Copyright (C) 2018 Peter Jones <pjones@redhat.com>
#
# Distributed under terms of the GPLv3 license.
#
#

set -euv

usage() {
    echo usage: $1 --branch '<origin_branch>' --repo '<origin_repo>' --remote '<remote_repo>' --pr-sha '<commit_id>'
    exit $2
}

declare origin_branch=""
declare origin_repo=""
declare remote_repo=""
declare pr_sha=""

let n=0 || :

if [[ $# -le 1 ]] ; then
    usage $0 1
fi

while [[ $# > 0 ]] ; do
    case " $1 " in
        " --help "|" -h "|" -? ")
            usage $0 0
            ;;
        " --branch ")
            origin_branch="$2"
            shift
            ;;
        " --repo ")
            origin_repo="$2"
            shift
            ;;
        " --remote ")
            remote_repo="$2"
            shift
            ;;
        " --pr-sha ")
            commit_id="$2"
            shift
            ;;
        *)
            usage $0 1
            ;;
    esac
    shift
done

if [[ -n "${remote_repo}" ]] ; then
    git remote add remote https://github.com/${remote_repo}
    git fetch remote
    git checkout -f ${commit_id}
fi
git reset --hard "${origin_branch}"
make EFIDIR=test ENABLE_SHIM_CERT=1 ENABLE_HTTBOOT=1 ARCH=x86_64 clean all
