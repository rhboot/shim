#!/bin/bash
#
# build.sh
# Copyright (C) 2018 Peter Jones <pjones@redhat.com>
#
# Distributed under terms of the GPLv3 license.
#
# shellcheck disable=SC2034

set -e
echo "${0}" "${@}"
set -x

cd /root

export EFIDIR=test
export ENABLE_HTTPBOOT=1

declare server_url="https://github.com"
declare event_type=""
declare origin_branch=""
declare origin_repo=""
declare commit_id=""
declare commit_range=""
declare pr="false"
declare pr_repo=""
declare pr_branch=""
declare pr_sha=""
declare test_subject="shim"

# pulling a patch from main-cp-devel into github-workflow-ci:
# GITHUB_ACTION=run3
# GITHUB_ACTIONS=true
# GITHUB_ACTOR=vathpela
# GITHUB_API_URL=https://api.github.com
# GITHUB_BASE_REF=github-workflow-ci
# GITHUB_ENV=/home/runner/work/_temp/_runner_file_commands/set_env_761b0172-6023-44a2-ad26-18047428f65b
# GITHUB_EVENT_NAME=pull_request
# GITHUB_EVENT_PATH=/home/runner/work/_temp/_github_workflow/event.json
# GITHUB_GRAPHQL_URL=https://api.github.com/graphql
# GITHUB_HEAD_REF=main-cp-devel
# GITHUB_JOB=do-build-rawhide
# GITHUB_PATH=/home/runner/work/_temp/_runner_file_commands/add_path_761b0172-6023-44a2-ad26-18047428f65b
# GITHUB_REF=refs/pull/240/merge
# GITHUB_REPOSITORY=rhboot/shim
# GITHUB_REPOSITORY_OWNER=rhboot
# GITHUB_RETENTION_DAYS=90
# GITHUB_RUN_ID=400987619
# GITHUB_RUN_NUMBER=14
# GITHUB_SERVER_URL=https://github.com
# GITHUB_SHA=e2a8442a4fc7275d7a6fd886695657a0b1b39ee3
# ^ no idea wth that's the sha of
# GITHUB_WORKFLOW='Build for CI'
# GITHUB_WORKSPACE=/home/runner/work/shim/shim
[[ -n "${GITHUB_HEAD_REF}" ]] && pr_branch="${GITHUB_HEAD_REF}"
[[ -n "${GITHUB_EVENT_NAME}" ]] && event_type="${GITHUB_EVENT_NAME}"
[[ -n "${GITHUB_BASE_REF}" ]] && origin_branch="${GITHUB_BASE_REF}"
[[ -n "${GITHUB_REF}" ]] && commit_id="${GITHUB_REF}"
[[ -n "${GITHUB_SHA}" ]] && pr_sha="${GITHUB_SHA}"
[[ -n "${GITHUB_REPOSITORY}" ]] && pr_repo="${GITHUB_REPOSITORY}"
[[ -n "${GITHUB_REPOSITORY}" ]] && origin_repo="${GITHUB_REPOSITORY}"
[[ -n "${GITHUB_SERVER_URL}" ]] && server_url="${GITHUB_SERVER_URL}"

set -u

remote_has_ref() {
    local remote="${1}" && shift
    local revision="${1}" && shift

    git show-ref -q "${remote:+${remote}/}${revision}"
}

if [[ "${GITHUB_ACTIONS}" = true ]] ; then
    if [[ -z "${pr_branch}" ]] ; then
        pr_branch="${commit_id##*/}"
    fi
    if [[ -z "${origin_branch}" ]] ; then
        origin_branch="${commit_id##*/}"
    fi
fi

do_test() {
    local subject="${1}" && shift
    local install="${1}" && shift
    local do_fetch="${1}" && shift
    if [[ $# -gt 0 ]] ; then
        local use_branch="${1}" && shift
    else
        local use_branch="${origin_branch}"
    fi

    cd "${subject}"
    if [[ "${do_fetch}" = yes ]] ; then
        if [[ "${event_type}" = "pull_request" ]] ; then
            git remote add remote "${server_url}/${pr_repo%%/*}/${subject}"
            if ! git fetch remote ; then
                git remote set-url remote "${server_url}/${origin_repo%%/*}/${subject}"
            fi
            if git fetch remote ; then
                if remote_has_ref remote "${pr_sha}" ; then
                    git clean -x -f
                    git clean -X -f
                    git checkout -f "remote/${pr_sha}"
                elif remote_has_ref remote "${pr_branch}" ; then
                    git clean -x -f
                    git clean -X -f
                    git checkout -f "remote/${pr_branch}"
                fi
            fi
        elif [[ "${event_type}" = "push" ]] ; then
            git fetch origin
            git reset --hard "origin/${use_branch}" --
        fi
    fi
    make PREFIX=/usr LIBDIR=/usr/lib64 clean all
    if [[ "${install}" = yes ]] ; then
        make PREFIX=/usr LIBDIR=/usr/lib64 install
        if [[ "${subject}" = gnu-efi ]] ; then
            mkdir -p /usr/lib64/gnuefi
            mv /usr/lib*/crt0-efi-*.o /usr/lib*/elf_*_efi.lds /usr/lib64/gnuefi/
        fi
    fi
    cd ..
}

export DESTDIR=/destdir
do_test "${test_subject}" yes yes
