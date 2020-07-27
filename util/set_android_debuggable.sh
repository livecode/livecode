#!/bin/bash

set -e

INPUT="$1"
OUTPUT="$2"

if [ "$BUILDTYPE" == "Debug" ] ; then
	sed 's/android:debuggable="false"/android:debuggable="true"/g' "${INPUT}" > "${OUTPUT}"
else
	cp "${INPUT}" "${OUTPUT}"
fi
