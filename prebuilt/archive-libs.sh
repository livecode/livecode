#!/bin/bash

# Package directory
PACKAGE_DIR="packaged"

PLATFORM=$1
ARCH=$2
SUBPLATFORM=$3

if [ -z "${SUBPLATFORM}" ] ; then
	ARCHIVE_TAR="${ARCH}-${PLATFORM}-prebuilts.tar"
else
	ARCHIVE_TAR="${ARCH}-${PLATFORM}-${SUBPLATFORM}-prebuilts.tar"
fi

echo "Creating archive ${ARCHIVE_TAR}"
# Add packages to archive
tar -cf "../${ARCHIVE_TAR}" "${PACKAGE_DIR}/"

if [ -f "../${ARCHIVE_TAR}" ] ; then
	bzip2 -zf --best "../${ARCHIVE_TAR}"
fi
