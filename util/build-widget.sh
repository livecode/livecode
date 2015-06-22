#!/bin/bash

set -e

function build_widget {
	WIDGET_DIR=$1
	WIDGET_NAME=$(basename $1)
	TARGET_DIR="com.livecode.extensions.livecode.${WIDGET_NAME}"
	BUILD_DIR=$2
	MODULE_DIR=$3
	LC_COMPILE=$4
	
	"${LC_COMPILE}" \
		--modulepath "${MODULE_DIR}" \
		--manifest "${WIDGET_DIR}/manifest.xml" \
		--output "${WIDGET_DIR}/module.lcm" \
		"${WIDGET_DIR}/${WIDGET_NAME}.lcb"
		
	pushd "${WIDGET_DIR}" 1>/dev/null
	zip -q -r "${TARGET_DIR}.lce" *
	popd 1>/dev/null
	
	mkdir -p "${BUILD_DIR}/packaged_extensions/${TARGET_DIR}"
	
	unzip -q \
		-o "${WIDGET_DIR}/${TARGET_DIR}.lce" \
		-d "${BUILD_DIR}/packaged_extensions/${TARGET_DIR}"
		
	rm "${WIDGET_DIR}/${TARGET_DIR}.lce"
	
	return 0
}

build_widget $@
