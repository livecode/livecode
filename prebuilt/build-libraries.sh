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
export CUSTOM_ARCH=$2

# Capture the existing CC and CXX variables, if any
export CUSTOM_CC="${CC}"
export CUSTOM_CXX="${CXX}"

# Special behaviour if "all" is specified as the platform
if [ "${PLATFORM}" == "all" ] ; then
	set -e
	"$0" mac
	"$0" android
	"$0" ios
	exit
fi

# Build each of the libraries that we depend on (OpenSSL, CURL and ICU)
$BASEDIR/scripts/build-curl.sh 
$BASEDIR/scripts/build-openssl.sh
$BASEDIR/scripts/build-icu.sh
