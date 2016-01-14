#!/bin/bash

# Utility script called by the build system to compile extensions

set -e

function extract_name {
	# Extract an e.g. "module foo.bar.baz" line from an LCB source file
	sed -nEe 's/^([[:space:]]*(module|widget|library)[[:space:]]*)([-_\.[:alnum:]]*)[[:space:]]*$/\3/p' < "$1"
}

function build_widget {
	WIDGET_DIR=$1
	WIDGET_NAME=$(basename $1)
	WIDGET_LCB="${WIDGET_DIR}/${WIDGET_NAME}.lcb"
	TARGET_DIR=$(extract_name "${WIDGET_LCB}")
	BUILD_DIR=$2
	MODULE_DIR=$3
	LC_COMPILE=$4
	
	"${LC_COMPILE}" \
		-Werror \
		--modulepath "${MODULE_DIR}" \
		--manifest "${WIDGET_DIR}/manifest.xml" \
		--output "${WIDGET_DIR}/module.lcm" \
		"${WIDGET_LCB}"

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

# Arguments 4 and above are the list of extensions to compile
destination_dir=$1
module_dir=$2
lc_compile=$3
shift 3

# Find the dependency/build ordering of the extensions
build_order=$(${lc_compile} --modulepath ${module_dir} --deps changed-order -- $@)

# Loop over the extensions that need to be (re-)built
for ext in ${build_order} ; do
	build_widget $(dirname "${ext}") "${destination_dir}" "${module_dir}" "${lc_compile}"
done
