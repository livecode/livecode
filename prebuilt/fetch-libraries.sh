#!/bin/bash

# Libraries to fetch
PLATFORMS=( mac linux win32 android ios emscripten )
ARCHS_android=( armv6 armv7 arm64 x86 x86_64 )
ARCHS_mac=( Universal )
ARCHS_ios=( Universal )
ARCHS_win32=( x86 x86_64 )
ARCHS_linux=( i386 x86_64 )
ARCHS_emscripten=( js )
LIBS_android=( OpenSSL ICU )
LIBS_mac=( OpenSSL ICU )
LIBS_ios=( OpenSSL ICU )
LIBS_win32=( OpenSSL Curl ICU CEF )
LIBS_linux=( OpenSSL Curl ICU CEF )
LIBS_emscripten=( ICU )

SUBPLATFORMS_ios=(iPhoneSimulator8.2 iPhoneSimulator9.2 iPhoneSimulator10.2 iPhoneSimulator11.2 iPhoneSimulator11.4 iPhoneOS9.2 iPhoneOS10.2 iPhoneOS11.2 iPhoneOS11.4)
SUBPLATFORMS_win32=(v140_static_debug v140_static_release)
SUBPLATFORMS_android=(api26)

# Fetch settings
SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
FETCH_DIR="${SCRIPT_DIR}/fetched"
EXTRACT_DIR="${SCRIPT_DIR}"
WIN32_EXTRACT_DIR="${SCRIPT_DIR}/unpacked"
URL="https://downloads.livecode.com/prebuilts"

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

	# Skip CEF on non-supported Linux platforms
	if [ "${PLATFORM}" == "linux" -a "${LIB}" == "CEF" ] ; then
		return
	fi

	eval "local VERSION=\${${LIB}_VERSION}"
	eval "local BUILDREVISION=\${${LIB}_BUILDREVISION}"

	# We now use standard GNU triple ordering for the naming of windows prebuilts
	local NAME=""
	if [ "${PLATFORM}" = "win32" ]; then
		NAME="${LIB}-${VERSION}-${ARCH}-${PLATFORM}-${SUBPLATFORM}"
	else
		NAME="${LIB}-${VERSION}-${PLATFORM}-${ARCH}"
		if [ ! -z "${SUBPLATFORM}" ] ; then
			NAME+="-${SUBPLATFORM}"
		fi
	fi
	
	if [ ! -z "${BUILDREVISION}" ] ; then
		NAME+="-${BUILDREVISION}"
	fi

	if [ ! -f "${FETCH_DIR}/${NAME}.tar.bz2" ]; then
		if [ -f "${LOCAL_DIR}/${NAME}.tar.bz2" ]; then
			echo "Fetching local library: ${NAME}"
			cp "${LOCAL_DIR}/${NAME}.tar.bz2" "${FETCH_DIR}/${NAME}.tar.bz2"
		else
			echo "Fetching remote library: ${NAME}"
		
			# Download using an HTTP client of some variety
			if $(which curl 1>/dev/null 2>/dev/null) ; then
				curl -k "${URL}/${NAME}.tar.bz2" -o "${FETCH_DIR}/${NAME}.tar.bz2" --fail
			elif $(which wget 1>/dev/null 2>/dev/null) ; then
				wget "${URL}/${NAME}.tar.bz2" -O "${FETCH_DIR}/${NAME}.tar.bz2"
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
			mkdir -p "${WIN32_EXTRACT_DIR}/${LIB}"
			cd "${WIN32_EXTRACT_DIR}/${LIB}"
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

	# TODO[2017-03-03] Our official architecture name (as used in
	# the gyp "toolset_os" variable) is "x86" rather than "i386",
	# but the prebuilt naming uniformly uses "i386".  Workaround
	# this by renaming the "i386" output folder to "x86"
	if [ -d "${EXTRACT_DIR}/lib/${PLATFORM}/i386" ] ; then
		if [ ! -d "${EXTRACT_DIR}/lib/${PLATFORM}/x86" ] ; then
			mkdir "${EXTRACT_DIR}/lib/${PLATFORM}/x86"
		fi
		cp -R "${EXTRACT_DIR}/lib/${PLATFORM}/i386/"* "${EXTRACT_DIR}/lib/${PLATFORM}/x86/"
		rm -r "${EXTRACT_DIR}/lib/${PLATFORM}/i386"
	fi

        # Windows-only hacks
	if [ "$PLATFORM" = "win32" ]; then

		# TODO[2017-02-17] Monkey-patch in a "Fast" prebuilt
		# The "Fast" configuration used by CI builds is identical to
		# the "Release" configuration, in terms of linkability.  So if
		# there's a "Release" build of any given library, duplicate
		# it for "Fast"
        for ARCH in "${ARCHS[@]}" ; do
                for LIB in "${LIBS[@]}" ; do
                        echo "Providing 'Fast' configuration for ${LIB} library"
                        for SUBPLATFORM in "${SUBPLATFORMS[@]}"; do
                                if [[ ! ${SUBPLATFORM} =~ _release$ ]]; then
                                        continue
                                fi
                                SRC_TRIPLE="${ARCH}-${PLATFORM}-${SUBPLATFORM}"
                                DST_TRIPLE=$(echo "${SRC_TRIPLE}" | sed 's/release/fast/')
                                SRC_DIR="${WIN32_EXTRACT_DIR}/${LIB}/${SRC_TRIPLE}"
                                DST_DIR="${WIN32_EXTRACT_DIR}/${LIB}/${DST_TRIPLE}"
                                if [[ -d "${SRC_DIR}" && ! -d "${DST_DIR}" ]]; then
                                        cp -a "${SRC_DIR}" "${DST_DIR}"
                                fi
                        done
                done
        done
                        
        for ARCH in "${ARCHS[@]}" ; do
                for LIB in "${LIBS[@]}" ; do
                        echo "Monkey patching toolset/arch for ${LIB} library"
                        for SUBPLATFORM in "v140_static_debug" "v140_static_release" "v140_static_fast"; do
                                SRC_TRIPLE="${ARCH}-${PLATFORM}-${SUBPLATFORM}"
                                DST_TRIPLE=$(echo "${SRC_TRIPLE}" | sed 's/v140/v141/')
                                SRC_DIR="${WIN32_EXTRACT_DIR}/${LIB}/${SRC_TRIPLE}"
                                DST_DIR="${WIN32_EXTRACT_DIR}/${LIB}/${DST_TRIPLE}"
                                if [[ -d "${SRC_DIR}" && ! -d "${DST_DIR}" ]]; then
                                        cp -a "${SRC_DIR}" "${DST_DIR}"
                                fi
                        done
                done
        done
	fi
done

# Don't forget the headers & data on non-Windows platforms
if [ 0 -eq "$FETCH_HEADERS" ]; then
	fetchLibrary OpenSSL All Universal Headers
	fetchLibrary ICU All Universal Headers
	fetchLibrary ICU All Universal Data
fi
