#!/bin/bash
#
# Run uncrustify over relevant parts of the source code

set -eu

declare -r TOP_SRCDIR=$(cd $(dirname $0)/.. && pwd)
declare -r UNCRUSTIFY=${UNCRUSTIFY:-uncrustify}

function list_files() {
    ${TOP_SRCDIR}/tools/uncrustify-files.py \
        ${TOP_SRCDIR} ${TOP_SRCDIR}/tools/uncrustify-files.json
}

function run_uncrustify() {
    ${UNCRUSTIFY} -c ${TOP_SRCDIR}/tools/uncrustify.cfg "$@"
}

function indent() {
    run_uncrustify --replace --no-backup -q -F <(list_files)
}

function check() {
    declare filename output count=0
    while read -r filename; do
        output="${filename}.uncrustify"
        run_uncrustify -q "${filename}"
        if ! cmp "${filename}" "${output}" > /dev/null; then
            echo "not ok - ${filename}"
            let ++count
        fi
	rm -f "${output}"
    done < <(list_files)
    if [[ "${count}" -ne 0 ]]; then
        echo "Incorrectly-formatted source code!"
        exit 1
    fi
}

function main() {
    if [[ $# > 0 ]] && [[ $1 = "--check" ]]; then
       check
    else
        indent
    fi
}

main "$@"
