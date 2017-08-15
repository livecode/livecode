#!/bin/bash

# Versions
source "scripts/lib_versions.inc"

# Package directory
PACKAGE_DIR="`pwd`/packaged"
mkdir -p "${PACKAGE_DIR}"

# Packager function
function doPackage {
	local PLATFORM=$1
	local ARCH=$2
	local SUBPLATFORM=$3
	
	# Linux x86 packages use i386 in their name (but still use x86 in the library folder!)
	if [ "${PLATFORM}" == "linux" -a "${ARCH}" == "x86" ] ; then
		local PACKAGE_ARCH=i386
	elif [ "${ARCH}" == "universal" ] ; then
		# watch case of "Universal" arch
		local PACKAGE_ARCH=Universal
	else
		local PACKAGE_ARCH="${ARCH}"
	fi

	echo "Creating packages: ${PLATFORM} ${ARCH} ${SUBPLATFORM}"

	if [ "${ARCH}" == "universal" ] ; then
		local ARCHDIR=
	else
		local ARCHDIR="${ARCH}"
	fi

	if [ ! -z "${SUBPLATFORM}" ] ; then
		local SUFFIX="-${PLATFORM}-${PACKAGE_ARCH}-${SUBPLATFORM}"
	else
		local SUFFIX="-${PLATFORM}-${PACKAGE_ARCH}"
	fi

	local LIBPATH="lib/${PLATFORM}/${ARCHDIR}/${SUBPLATFORM}"

	local OPENSSL_TAR="${PACKAGE_DIR}/OpenSSL-${OpenSSL_VERSION}${SUFFIX}.tar"
	local CURL_TAR="${PACKAGE_DIR}/Curl-${Curl_VERSION}${SUFFIX}.tar"
	local ICU_TAR="${PACKAGE_DIR}/ICU-${ICU_VERSION}${SUFFIX}.tar"
	local CEF_TAR="${PACKAGE_DIR}/CEF-${CEF_VERSION}${SUFFIX}.tar"

	# Package up OpenSSL
	if [ -f "${LIBPATH}/libcustomcrypto.a" ] ; then
		tar -cf "${OPENSSL_TAR}" "${LIBPATH}/libcustomcrypto.a" "${LIBPATH}/libcustomssl.a"
	elif [ -f "${LIBPATH}/revsecurity.dll" ] ; then
		tar -cf "${OPENSSL_TAR}" "${LIBPATH}/libeay32.lib" "${LIBPATH}/ssleay32.lib" "${LIBPATH}/revsecurity.dll" "${LIBPATH}/revsecurity.def"
	fi

	# Package up Curl
	if [ -f "${LIBPATH}/libcurl.a" ] ; then
		tar -cf "${CURL_TAR}" "${LIBPATH}/libcurl.a"
	elif [ -f "${LIBPATH}/libcurl_a.lib" ] ; then
		tar -cf "${CURL_TAR}" "${LIBPATH}/libcurl_a.lib"
	fi

	# Package up ICU
	local ICU_LIBS=
	if [ -f "${LIBPATH}/libicudata.a" ] ; then
		for LIB in data i18n io le lx tu uc ; do
			if [ -f "${LIBPATH}/libicu${LIB}.a" ] ; then
				ICU_LIBS+="${LIBPATH}/libicu${LIB}.a "
			fi	
		done

		tar -cf "${ICU_TAR}" ${ICU_LIBS}

	elif [ -f "${LIBPATH}/sicudt.lib" ] ; then
		for LIB in dt in io le lx tu uc ; do
			if [ -f "${LIBPATH}/sicu${LIB}.lib" ] ; then
				ICU_LIBS+="${LIBPATH}/sicu${LIB}.lib "
			fi
		done

		tar -cf "${ICU_TAR}" ${ICU_LIBS}
	fi

	# Package up CEF
	if [ "$PLATFORM" = "win32" -o "$PLATFORM" = "linux" ] ; then
		if [ -f "${LIBPATH}/CEF" ] ; then
			tar -cf "${CEF_TAR}" "${LIBPATH}/CEF"
		fi
	fi

	# Compress the packages
	if [ -f "${OPENSSL_TAR}" ] ; then
		bzip2 -zf --best "${OPENSSL_TAR}"
	fi
	if [ -f "${CURL_TAR}" ] ; then
		bzip2 -zf --best "${CURL_TAR}"
	fi
	if [ -f "${ICU_TAR}" ] ; then
		bzip2 -zf --best "${ICU_TAR}"
	fi
	if [ -f "${CEF_TAR}" ] ; then
		bzip2 -zf --best "${CEF_TAR}"
	fi
}

PLATFORM=$1
ARCH=$2

#only ios subplatforms are used
if [ "${PLATFORM}" = "ios" ] ; then
	SUBPLATFORM=$3
else
	SUBPLATFORM=
fi

# Package up the various libraries and headers
doPackage "${PLATFORM}" "${ARCH}" "${SUBPLATFORM}"

# We only need shared headers to be packaged once, so only do this on linux-x86_64
if [ "${PLATFORM}" = "linux" -a "${ARCH}" = "x86_64" ] ; then
	# Package up the includes
	OPENSSL_HDR_TAR="${PACKAGE_DIR}/OpenSSL-${OpenSSL_VERSION}-All-Universal-Headers.tar"
	ICU_HDR_TAR="${PACKAGE_DIR}/ICU-${ICU_VERSION}-All-Universal-Headers.tar"
	tar -cf "${OPENSSL_HDR_TAR}" include/openssl/*.h
	tar -cf "${ICU_HDR_TAR}" include/unicode/*.h
	bzip2 -zf --best "${OPENSSL_HDR_TAR}"
	bzip2 -zf --best "${ICU_HDR_TAR}"
fi
