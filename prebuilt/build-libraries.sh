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

#only mac, ios subplatforms are used
if [ "${PLATFORM}" = "mac" -o "${PLATFORM}" = "ios" ] ; then
	export SUBPLATFORM=$3
else
	export SUBPLATFORM=
fi

# Capture the existing CC and CXX variables, if any
export CUSTOM_CC="${CC}"
export CUSTOM_CXX="${CXX}"

# Build all of the libraries that we depend on (OpenSSL, CURL and ICU) if not specified
$BASEDIR/scripts/build-openssl.sh 
$BASEDIR/scripts/build-curl.sh
$BASEDIR/scripts/build-icu.sh
