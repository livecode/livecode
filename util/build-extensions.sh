#!/bin/bash

# Utility script called by the build system to compile extensions

set -eu

function err() {
	echo "ERROR: $@" >&2
}

function extract_name {
	# Extract an e.g. "module foo.bar.baz" line from an LCB source file
	sed -nEe 's,^([[:space:]]*<name>(.*)</name>[[:space:]]*)$,\2,p' < "$1"
}

function do_cmd {
	if [[ ! -z "${V}" ]]; then
		echo "$@"
	fi
	"$@"
}

function build_widget {
	local WIDGET_DIR=$1
	local WIDGET_NAME=$(basename $1)
	local WIDGET_LCB="${WIDGET_DIR}/${WIDGET_NAME}.lcb"
	local BUILD_DIR=$2
	local MODULE_DIR=$3
	local LC_COMPILE=$4
	
	echo "  LC_COMPILE ${WIDGET_DIR}/module.lcm"

	do_cmd "${LC_COMPILE}" \
		-Werror \
		--modulepath "${MODULE_DIR}" \
		--manifest "${WIDGET_DIR}/manifest.xml" \
		--output "${WIDGET_DIR}/module.lcm" \
		"${WIDGET_LCB}"

	local TARGET_DIR=$(extract_name "${WIDGET_DIR}/manifest.xml")
	if [[ -z "${TARGET_DIR}" ]]; then
		err "Could not find canonical name of ${WIDGET_NAME}"
		return 1
	fi

	echo "  PACKAGE ${TARGET_DIR}"

	pushd "${WIDGET_DIR}" 1>/dev/null
	zip -q -r "${TARGET_DIR}.lce" *
	popd 1>/dev/null
	
	mkdir -p "${BUILD_DIR}/${TARGET_DIR}"
	
	unzip -q \
		-o "${WIDGET_DIR}/${TARGET_DIR}.lce" \
		-d "${BUILD_DIR}/${TARGET_DIR}"
		
	rm "${WIDGET_DIR}/${TARGET_DIR}.lce"
	
	return 0
}

# Detect verbose mode
V=${V:-}

# Arguments 4 and above are the list of extensions to compile
readonly destination_dir=$1
readonly module_dir=$2
readonly lc_compile=$3
shift 3

# Find the dependency/build ordering of the extensions
readonly build_order=$(${lc_compile} --modulepath ${module_dir} --deps changed-order -- $@)

# Loop over the extensions that need to be (re-)built
for ext in ${build_order} ; do
	build_widget $(dirname "${ext}") "${destination_dir}" "${module_dir}" "${lc_compile}"
done
