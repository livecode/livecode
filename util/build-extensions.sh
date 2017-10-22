#!/bin/bash

# Utility script called by the build system to compile extensions

set -eu

function err() {
	echo "ERROR: $@" >&2
}

function do_cmd {
	if [[ ! -z "${V}" ]]; then
		echo "$@"
	fi
	"$@"
}

function build_widget {
    do_cmd "$1" "$2" "'dummy1'" "'dummy2'" "'dummy3'" "$3" "$4" \
        $(dirname "${5}") $(basename "${5}") \
        "$6" "$7" "$8" "$9"
    
	return 0
}

# Detect verbose mode
V=${V:-}

# Arguments 9 and above are the list of extensions to compile
readonly server_engine=$1
readonly packager=$2
readonly docs_extractor=$3
readonly docs_parser=$4
readonly destination_dir=$5
readonly remove_src=$6
readonly lc_compile=$7
readonly module_dir=$8

shift 8

# Find the dependency/build ordering of the extensions
readonly build_order=$(${lc_compile} --modulepath ${module_dir} --deps order -- $@)

# Loop over the extensions that need to be (re-)built
for ext in ${build_order} ; do
	build_widget "${server_engine}" "${packager}" \
	    "${docs_extractor}" "${docs_parser}" \
	    "${ext}" \
	    "${destination_dir}" "${remove_src}" "${lc_compile}" \
	    "${module_dir}"
	    
done
