#!/bin/bash

if [ -e "${PLATFORM_DEVELOPER_BIN_DIR}/g++" ] ; then
	BIN_DIR="${PLATFORM_DEVELOPER_BIN_DIR}"
else
	BIN_DIR="${DEVELOPER_BIN_DIR}"
fi

if [ "${CONFIGURATION}" = "Debug" ] ; then
	STRIP_FLAG="-Wl,-x"
else
	STRIP_FLAG=""
fi

"${BIN_DIR}/g++" -nodefaultlibs -Wl,-r ${STRIP_FLAG} -arch ${ARCHS//\ /\ -arch\ } -isysroot "${SDKROOT}" -o $@ -Wl,-sectcreate -Wl,-__MISC -Wl,__deps -Wl,"${SRCROOT}/standalone.ios" -Wl,-exported_symbol -Wl,_main -Wl,-exported_symbol -Wl,_load_module -Wl,-exported_symbol -Wl,_resolve_symbol

