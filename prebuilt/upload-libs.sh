#!/bin/bash

# Versions
source "scripts/lib_versions.inc"

# Package directory
PACKAGE_DIR="`pwd`/packaged"

PLATFORM=$1
ARCH=$2

#only ios subplatforms are used
if [ "${PLATFORM}" = "ios" ] ; then
	SUBPLATFORM=$3
else
	SUBPLATFORM=
fi

# Linux x86 packages use i386 in their name (but still use x86 in the library folder!)
if [ "${PLATFORM}" == "linux" -a "${ARCH}" == "x86" ] ; then
	PACKAGE_ARCH=i386
else
	PACKAGE_ARCH="${ARCH}"
fi

if [ ! -z "${SUBPLATFORM}" ] ; then
	SUFFIX="-${PLATFORM}-${PACKAGE_ARCH}-${SUBPLATFORM}"
else
	SUFFIX="-${PLATFORM}-${PACKAGE_ARCH}"
fi

OPENSSL_PKG="${PACKAGE_DIR}/OpenSSL-${OpenSSL_VERSION}${SUFFIX}.tar.bz2"
CURL_PKG="${PACKAGE_DIR}/Curl-${Curl_VERSION}${SUFFIX}.tar.bz2"
ICU_PKG="${PACKAGE_DIR}/ICU-${ICU_VERSION}${SUFFIX}.tar.bz2"
CEF_PKG="${PACKAGE_DIR}/CEF-${CEF_VERSION}${SUFFIX}.tar.bz2"
OPENSSL_HDR_PKG="${PACKAGE_DIR}/OpenSSL-${OpenSSL_VERSION}-All-Universal-Headers.tar.bz2"
ICU_HDR_PKG="${PACKAGE_DIR}/ICU-${ICU_VERSION}-All-Universal-Headers.tar.bz2"

PACKAGE_FILES=

if [ -f "${OPENSSL_PKG}" ] ; then
	PACKAGE_FILES+="${OPENSSL_PKG} "
fi
if [ -f "${CURL_PKG}" ] ; then
	PACKAGE_FILES+="${CURL_PKG} "
fi
if [ -f "${ICU_PKG}" ] ; then
	PACKAGE_FILES+="${ICU_PKG} "
fi
if [ -f "${CEF_PKG}" ] ; then
	PACKAGE_FILES+="${CEF_PKG} "
fi
if [ -f "${OPENSSL_HDR_PKG}" ] ; then
	PACKAGE_FILES+="${OPENSSL_HDR_PKG} "
fi
if [ -f "${ICU_HDR_PKG}" ] ; then
	PACKAGE_FILES+="${ICU_HDR_PKG} "
fi

echo "Uploading packages: ${PLATFORM} ${ARCH} ${SUBPLATFORM}"

# Upload settings
UPLOAD_SERVER="meg.on-rev.com"
# TESTING - using test_upload folder while testing
UPLOAD_FOLDER="prebuilts/test_uploads/"
# TESTING - reduced retry count while testing
UPLOAD_MAX_RETRIES=3
#UPLOAD_MAX_RETRIES=50

trap "echo Interrupted; exit 1;" SIGINT SIGTERM
i=0
false
while [ $? -ne 0 -a $i -lt $UPLOAD_MAX_RETRIES ] ; do
	i=$(($i+1))
	rsync -v --progress --chmod=ug=rw,o=r --partial ${PACKAGE_FILES} "${UPLOAD_SERVER}:${UPLOAD_FOLDER}"
done
rc=$?
if [ $rc -ne 0 ]; then
	echo "Maximum retries reached, giving up"
fi
exit $rc

