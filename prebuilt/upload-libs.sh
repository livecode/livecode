#!/bin/bash

# Versions
source "scripts/lib_versions.inc"

# Package directory
PACKAGE_DIR="packaged"

echo "Uploading packages"

# Upload settings
UPLOAD_SERVER="meg.on-rev.com"
# TESTING - using test_upload folder while testing
UPLOAD_FOLDER="prebuilts/test_uploads/"
# TESTING - reduced retry count while testing
UPLOAD_MAX_RETRIES=3
#UPLOAD_MAX_RETRIES=50

cd "${PACKAGE_DIR}"
find . -type f -name "*.tar.*" > prebuilts-upload-files.txt

trap "echo Interrupted; exit 1;" SIGINT SIGTERM
i=0
false
while [ $? -ne 0 -a $i -lt $UPLOAD_MAX_RETRIES ] ; do
	i=$(($i+1))
	rsync -v --progress --chmod=ug=rw,o=r --partial --files-from=prebuilts-upload-files.txt . "${UPLOAD_SERVER}:${UPLOAD_FOLDER}"
done
rc=$?
if [ $rc -ne 0 ]; then
	echo "Maximum retries reached, giving up"
	exit $rc
fi
