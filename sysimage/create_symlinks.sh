#!/bin/sh

# Copyright (c)  Intel Corporation.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
# * Redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in
# the documentation and/or other materials provided with the
# distribution.
# * Neither the name of Intel Corporation nor the names of its
# contributors may be used to endorse or promote products derived
# from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

set -e
# set -x

die()
{
    >&2 printf "$@"
    exit 1
}

warn()
{
    >&2 printf 'warning: '
    >&2 printf "$@"
}

for d in spi-flash-tools Quark_EDKII sysimage meta-clanton quark_linux grub-legacy
do
    printf 'See if we can: ln -s  ./%s_*  %s\n' "$d" "$d"
    # Check for multiple matches
    count=0
    for e in "${d}"_* ; do
        if test -d "$e" &&
	     # Some directory like this is created by EDKII/svn_setup.py: must exclude
	    test x"${e%svn*externals*repo}" = x"${e}"
	then
	    printf 'Found %s\n' "$e"
            count=$(( count+1 ))
	    srcdir="$e"
        fi
    done

    if [ $count -eq 0 ] ; then
        continue
    elif [ $count -gt 1 ] ; then
        warn '%s\n' "fail: multiple matches on directory ${d}_*" 
	continue
    fi

    # Check for existing symlinks
    if [ -h "${d}" ]; then
        warn '%s\n' "fail: symlink already exists: ${d}"
	continue
    fi

    # Create the symlink
    ( set -x
    ln -s "${srcdir}" "${d}" ) || true
done

