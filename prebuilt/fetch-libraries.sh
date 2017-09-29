#!/bin/bash

# Libraries to fetch
PLATFORMS=( mac linux win32 android ios emscripten )
ARCHS_android=( armv6 )
ARCHS_mac=( Universal )
ARCHS_ios=( Universal )
ARCHS_win32=( i386 )
ARCHS_linux=( i386 x86_64 )
ARCHS_emscripten=( js )
LIBS_android=( OpenSSL ICU )
LIBS_mac=( OpenSSL ICU )
LIBS_ios=( OpenSSL ICU )
LIBS_win32=( OpenSSL Curl ICU CEF )
LIBS_linux=( OpenSSL Curl ICU CEF )
LIBS_emscripten=( ICU )
SUBPLATFORMS_ios=(iPhoneSimulator6.1 iPhoneSimulator7.1 iPhoneSimulator8.2 iPhoneSimulator9.2 iPhoneSimulator10.2 iPhoneSimulator11.0 iPhoneOS9.2 iPhoneOS10.2 iPhoneOS11.0)

# Fetch settings
SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
FETCH_DIR="${SCRIPT_DIR}/fetched"
EXTRACT_DIR="${SCRIPT_DIR}"
URL="http://downloads.livecode.com/prebuilts"

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

	local NAME="${LIB}-${VERSION}-${PLATFORM}-${ARCH}"

	if [ ! -z "${SUBPLATFORM}" ] ; then
		NAME+="-${SUBPLATFORM}"
	fi

	if [ ! -f "${FETCH_DIR}/${NAME}.tar.bz2" ] ; then
		echo "Fetching library: ${NAME}"
		
		# Download using an HTTP client of some variety
		if $(which curl 1>/dev/null 2>/dev/null) ; then
			curl --silent "${URL}/${NAME}.tar.bz2" -o "${FETCH_DIR}/${NAME}.tar.bz2"
		elif $(which wget 1>/dev/null 2>/dev/null) ; then
			wget "${URL}/${NAME}.tar.bz2" -O "${FETCH_DIR}/${NAME}.tar.bz2"
		else
			# Perl as a last resort (useful for Cygwin)
			perl -MLWP::Simple -e "getstore('${URL}/${NAME}.tar.bz2', '${FETCH_DIR}/${NAME}.tar.bz2') == 200 or exit 1"
		fi

		if [ $? -ne 0 ] ; then
			echo "    failed"
			exit 1
		fi

		echo "Extracting library: ${NAME}"
		DIR="`pwd`"
		cd "${EXTRACT_DIR}"
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

for PLATFORM in ${SELECTED_PLATFORMS} ; do
	# Work around an issue where Gyp is too enthusiastic in path-ifying arguments
	PLATFORM=$(basename "$PLATFORM")

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
	
	# Re-name the "i386" output folder to "x86"
	if [ -d "${EXTRACT_DIR}/lib/${PLATFORM}/i386" ] ; then
		if [ ! -d "${EXTRACT_DIR}/lib/${PLATFORM}/x86" ] ; then
			mkdir "${EXTRACT_DIR}/lib/${PLATFORM}/x86"
		fi
		cp -R "${EXTRACT_DIR}/lib/${PLATFORM}/i386/"* "${EXTRACT_DIR}/lib/${PLATFORM}/x86/"
		rm -r "${EXTRACT_DIR}/lib/${PLATFORM}/i386"
	fi
done

# Don't forget the headers
fetchLibrary OpenSSL All Universal Headers
fetchLibrary ICU All Universal Headers

