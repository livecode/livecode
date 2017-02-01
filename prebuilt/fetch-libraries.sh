#!/bin/bash

# Libraries to fetch
PLATFORMS=( mac linux win32 android ios emscripten )
ARCHS_android=( armv6 )
ARCHS_mac=( Universal )
ARCHS_ios=( Universal )
ARCHS_win32=( x86 )
ARCHS_linux=( i386 x86_64 )
ARCHS_emscripten=( js )
LIBS_android=( OpenSSL ICU )
LIBS_mac=( OpenSSL ICU )
LIBS_ios=( OpenSSL ICU )
LIBS_win32=( OpenSSL Curl ICU CEF )
LIBS_linux=( OpenSSL Curl ICU CEF )
LIBS_emscripten=( ICU )
SUBPLATFORMS_ios=(iPhoneSimulator6.1 iPhoneSimulator7.1 iPhoneSimulator8.2 iPhoneSimulator9.2 iPhoneSimulator10.2 iPhoneOS9.2 iPhoneOS10.2)
SUBPLATFORMS_win32=(v140_static_debug v140_static_release)

# Fetch settings
SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
FETCH_DIR="${SCRIPT_DIR}/fetched"
EXTRACT_DIR="${SCRIPT_DIR}"
WIN32_EXTRACT_DIR="${SCRIPT_DIR}/unpacked"
URL="http://downloads.livecode.com/prebuilts"

# Platform specific settings
if [ "${OS}" = "Windows_NT" ]; then
	LOCAL_DIR=/cygdrive/c/LiveCode/Prebuilt/libraries
	if [ ! -e "${LOCAL_DIR}" ]; then
		LOCAL_DIR=
	fi
else
	LOCAL_DIR=
fi

# Versions
source "${SCRIPT_DIR}/scripts/lib_versions.inc"

mkdir -p "${FETCH_DIR}"
mkdir -p "${EXTRACT_DIR}"

# Override the fetch location via an environment variable
if [ ! -z "${PREBUILT_CACHE_DIR}" ] ; then
    FETCH_DIR="${PREBUILT_CACHE_DIR}"
fi

function fetchLibrary {
	local LIB=$1
	local PLATFORM=$2
	local ARCH=$3
	local SUBPLATFORM=$4

	eval "local VERSION=\${${LIB}_VERSION}"

	# We now use standard GNU triple ordering for the naming of windows prebuilts
	local NAME=""
	if [ "${PLATFORM}" = "win32" ]; then
		if [ "${LIB}" = "CEF" ]; then
			NAME="CEF-${VERSION}-win32-i386"
		else
			NAME="${LIB}-${VERSION}-${ARCH}-${PLATFORM}-${SUBPLATFORM}"
		fi
	else
		NAME="${LIB}-${VERSION}-${PLATFORM}-${ARCH}"
		if [ ! -z "${SUBPLATFORM}" ] ; then
			NAME+="-${SUBPLATFORM}"
		fi
	fi

	if [ ! -f "${FETCH_DIR}/${NAME}.tar.bz2" ]; then
		if [ -f "${LOCAL_DIR}/${NAME}.tar.bz2" ]; then
			echo "Fetching local library: ${NAME}"
			cp "${LOCAL_DIR}/${NAME}.tar.bz2" "${FETCH_DIR}/${NAME}.tar.bz2"
		else
			echo "Fetching remote library: ${NAME}"
		
			# Download using an HTTP client of some variety
			if $(which curl 1>/dev/null 2>/dev/null) ; then
				curl --silent "${URL}/${NAME}.tar.bz2" -o "${FETCH_DIR}/${NAME}.tar.bz2"
			elif $(which wget 1>/dev/null 2>/dev/null) ; then
				get "${URL}/${NAME}.tar.bz2" -O "${FETCH_DIR}/${NAME}.tar.bz2"
			else
				# Perl as a last resort (useful for Cygwin)
				perl -MLWP::Simple -e "getstore('${URL}/${NAME}.tar.bz2', '${FETCH_DIR}/${NAME}.tar.bz2') == 200 or exit 1"
			fi
		fi

		if [ ! -e "${FETCH_DIR}/${NAME}.tar.bz2" ]; then
			echo "Failed to find library ${NAME} either remotely or locally"
			exit 1
		fi

		echo "Extracting library: ${NAME}"
		DIR="`pwd`"
		if [ "${PLATFORM}" = "win32" ]; then
			if [ "${LIB}" = "CEF" ]; then
				cd "${EXTRACT_DIR}"
			else
				mkdir -p "${WIN32_EXTRACT_DIR}/${LIB}"
				cd "${WIN32_EXTRACT_DIR}/${LIB}"
			fi
		else
			cd "${EXTRACT_DIR}"
		fi
		tar -jxf "${FETCH_DIR}/${NAME}.tar.bz2"
		RESULT=$?
		cd "${DIR}"
		if [ "${RESULT}" -ne 0 ] ; then
			echo "    failed"
			exit 1
		fi

	else
		echo "Already fetched: ${NAME}"
	fi
}

if [ 0 -eq "$#" ]; then
    SELECTED_PLATFORMS="${PLATFORMS[@]}"
else
    SELECTED_PLATFORMS="$@"
fi

FETCH_HEADERS=0

for PLATFORM in ${SELECTED_PLATFORMS} ; do
	# Work around an issue where Gyp is too enthusiastic in path-ifying arguments
	PLATFORM=$(basename "$PLATFORM")

	# Windows prebuilts now include their headers
	if [ "$PLATFORM" = "win32" ]; then
		FETCH_HEADERS=1
	fi

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
	
	# Re-name the "i386" output folder to "x86" but only for non-windows
	if [ "$PLATFORM" = "win32" ]; then
		if [ -d "${EXTRACT_DIR}/lib/${PLATFORM}/i386" ] ; then
			if [ ! -d "${EXTRACT_DIR}/lib/${PLATFORM}/x86" ] ; then
				mkdir "${EXTRACT_DIR}/lib/${PLATFORM}/x86"
			fi
			cp -R "${EXTRACT_DIR}/lib/${PLATFORM}/i386/"* "${EXTRACT_DIR}/lib/${PLATFORM}/x86/"
			rm -r "${EXTRACT_DIR}/lib/${PLATFORM}/i386"
		fi
	fi
done

# Don't forget the headers on non-Windows platforms
if [ 0 -eq "$FETCH_HEADERS" ]; then
	fetchLibrary OpenSSL All Universal Headers
	fetchLibrary ICU All Universal Headers
fi
