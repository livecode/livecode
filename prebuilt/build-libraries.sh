#!/bin/bash

# Abort building if any errors occur
set -e

# Build location
export BASEDIR=$(dirname $0)
export BUILDDIR="`pwd`/build"
export INSTALL_DIR="`pwd`/build/install"
export OUTPUT_DIR="`pwd`"
mkdir -p "${BUILDDIR}"
mkdir -p "${INSTALL_DIR}"
mkdir -p "${OUTPUT_DIR}"

# Target platform and architecture
export PLATFORM=$1
export ARCH=$2

#only ios subplatforms are used
if [ "${PLATFORM}" = "ios" ] ; then
	export SUBPLATFORM=$3
else
	export SUBPLATFORM=
fi

# Capture the existing build variables, if any
export CUSTOM_CC="${CC}"
export CUSTOM_CXX="${CXX}"
export CUSTOM_EMMAKE="${EMMAKE}"
export CUSTOM_EMCONFIGURE="${EMCONFIGURE}"

# Set which libs to build for the target platform
case "${PLATFORM}" in
	android)
		PREBUILT_LIBS="openssl icu"
		;;
	mac)
		PREBUILT_LIBS="openssl icu"
		;;
	ios)
		PREBUILT_LIBS="openssl icu"
		;;
	win32)
		PREBUILT_LIBS="openssl curl icu"
		;;
	linux)
		PREBUILT_LIBS="openssl curl icu"
		;;
	emscripten)
		PREBUILT_LIBS="icu"
		;;
esac

# Build all of the libraries that the target platform depends on
for t_lib in ${PREBUILT_LIBS} ; do
	${BASEDIR}/scripts/build-${t_lib}.sh
done
