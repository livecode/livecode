#!/bin/bash
#
# Remove old release notes that are probably no longer relevant

set -eu

declare -r TOP_SRCDIR=$(cd $(dirname $0)/.. && pwd)

IFS=;

# Print an error message to stderr
function error() {
    echo "ERROR: $1" >/dev/stderr
    exit 1
}

# Output the SHA-1 of the most recent commit touching a path to stdout
function get_latest_file_commit() {
    local path
    path=$1

    if [[ -z "${path}" ]]; then
        error "Refusing to get latest commit for empty path"
    fi

    local directory
    directory=$(dirname "${path}")

    (cd "${directory}" &&
            git log --topo-order -n1 -- "${path}" | head -n1 | cut -d" " -f2 )
}

# Output the tags that contain a particular commit SHA-1
function get_tags_with_commit() {
    local commit
    commit=$1

    if [[ -z "${commit}" ]]; then
        error "Cannot get tags containing empty commit"
    fi

    local directory
    directory=$(dirname "${path}")

    (cd "${directory}" && git tag --contains "${commit}") ||
            error "Cannot get tags for ${commit}"
}

# Output the newest major version in which a file was modified
function get_latest_file_major_version() {
    local file
    file=$1

    if [[ -z "${file}" ]]; then
        error "Cannot get tags containing empty commit"
    fi

    local commit
    commit=$(get_latest_file_commit "${file}")
    if [[ -z "${commit}" ]]; then
        return
    fi

    get_tags_with_commit "${commit}" |
        cut -d"." -f1 | sort -n | head -n1
}

# Clean a single file
#
# Globals:
#   MAJOR_VERSION
function clean_file() {
    local file
    file=$1

    local major_version
    major_version=$(get_latest_file_major_version "${file}")

    if [[ -n "${major_version}" ]] &&
           [[ "${major_version}" -lt "${MAJOR_VERSION}" ]]; then
        rm "${file}"
        echo "removed (${major_version}) - ${file}" >&2
    else
        echo "kept (${major_version}) - ${file}" >&2
    fi
}

# Clean a single directory
function clean_directory() {
    local path
    path=$1

    if [[ -z "${path}" ]]; then
        error "Refusing to clean empty path"
    fi
    while read -r -d $'\0' filename; do
        clean_file "${filename}"
    done < <(find "${path}" -type f -print0)
}

function main() {
    declare -rg MAJOR_VERSION=$(perl ${TOP_SRCDIR}/util/decode_version.pl \
                                     BUILD_MAJOR_VERSION ${TOP_SRCDIR}/version)

    while read -r -d $'\0' directory; do
        clean_directory "${directory}"
    done < <(find "${TOP_SRCDIR}" -type d -name notes -print0)
}

main "$@"
