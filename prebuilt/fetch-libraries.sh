#!/bin/bash

# Library versions
VERSION_OpenSSL="1.0.1g"
VERSION_Curl="7.21.1"
VERSION_ICU="52.1"

# Libraries to fetch
PLATFORMS=( mac linux ios win32 android )
ARCHS_android=( armv6 )
ARCHS_mac=( Universal )
ARCHS_ios=( Universal )
ARCHS_win32=( i386 )
ARCHS_linux=( i386 x86_64 )
LIBS_android=( OpenSSL ICU )
LIBS_mac=( OpenSSL ICU )
LIBS_ios=( OpenSSL ICU )
LIBS_win32=( OpenSSL Curl ICU )
LIBS_linux=( OpenSSL Curl ICU )
SUBPLATFORMS_ios=( iPhoneSimulator4.3 iPhoneSimulator5.0 iPhoneSimulator5.1 iPhoneSimulator6.0 iPhoneSimulator6.1 iPhoneSimulator7.0 iPhoneSimulator7.1 iPhoneOS6.1 iPhoneOS7.0 iPhoneOS7.1 )

# Fetch settings
FETCH_DIR=`pwd`/fetched
EXTRACT_DIR=`pwd`
URL="http://downloads.livecode.com/prebuilts"

mkdir -p "${FETCH_DIR}"
mkdir -p "${EXTRACT_DIR}"

function fetchLibrary {
	local LIB=$1
	local PLATFORM=$2
	local ARCH=$3
	local SUBPLATFORM=$4

	eval "local VERSION=\${VERSION_${LIB}}"

	local NAME="${LIB}-${VERSION}-${PLATFORM}-${ARCH}"

	if [ ! -z "${SUBPLATFORM}" ] ; then
		NAME+="-${SUBPLATFORM}"
	fi

	echo "Fetching library: ${NAME}"
	curl --silent "${URL}/${NAME}.tar.bz2" -o "${FETCH_DIR}/${NAME}.tar.bz2"
	#cp "`pwd`/packaged/${NAME}.tar.bz2" "${FETCH_DIR}/${NAME}.tar.bz2"
	if [ $? -ne 0 ] ; then
		echo "    failed"
		exit
	fi

	echo "Extracting library: ${NAME}"
	DIR="`pwd`"
	cd "${EXTRACT_DIR}"
	tar -jxf "${FETCH_DIR}/${NAME}.tar.bz2"
	RESULT=$?
	cd "${DIR}"
	if [ "${RESULT}" -ne 0 ] ; then
		echo "    failed"
		exit
	fi
}

for PLATFORM in "${PLATFORMS[@]}" ; do
	eval "ARCHS=( \${ARCHS_${PLATFORM}[@]} )"
	eval "LIBS=( \${LIBS_${PLATFORM}[@]} )"
	eval "SUBPLATFORMS=( \${SUBPLATFORMS_${PLATFORM}[@]} )"

	for ARCH in "${ARCHS[@]}" ; do
		for LIB in "${LIBS[@]}" ; do
			if [ ! -z "${SUBPLATFORMS}" ] ; then
				for SUBPLATFORM in "${SUBPLATFORMS[@]}" ; do
					fetchLibrary "${LIB}" "${PLATFORM}" "${ARCH}" "${SUBPLATFORM}"
				done
			else
				fetchLibrary "${LIB}" "${PLATFORM}" "${ARCH}"
			fi
		done
	done
done

# Don't forget the headers
fetchLibrary OpenSSL All Universal Headers
fetchLibrary ICU All Universal Headers

