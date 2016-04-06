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

	echo "Creating packages: ${PLATFORM} ${ARCH} ${SUBPLATFORM}"

	if [ "${ARCH}" == "Universal" ] ; then
		local ARCHDIR=
	else
		local ARCHDIR="${ARCH}"
	fi

	if [ ! -z "${SUBPLATFORM}" ] ; then
		local SUFFIX="-${PLATFORM}-${ARCH}-${SUBPLATFORM}"
	else
		local SUFFIX="-${PLATFORM}-${ARCH}"
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
		tar -cf "${CEF_TAR}" "${LIBPATH}/CEF"
	fi

	# Compress the packages
	if [ -f "${OPENSSL_TAR}" ] ; then
		bzip2 -z --best "${OPENSSL_TAR}"
	fi
	if [ -f "${CURL_TAR}" ] ; then
		bzip2 -z --best "${CURL_TAR}"
	fi
	if [ -f "${ICU_TAR}" ] ; then
		bzip2 -z --best "${ICU_TAR}"
	fi
	if [ -f "${CEF_TAR}" ] ; then
		bzip2 -z --best "${CEF_TAR}"
	fi
}

# Package up the various libraries and headers
for PLATFORM in `find lib/ -mindepth 1 -maxdepth 1 -type d` ; do
	PLATFORM=$(basename "${PLATFORM}")
	if [ "${PLATFORM}" == "mac" ] ; then
		doPackage "${PLATFORM}" "Universal" 
	elif [ "${PLATFORM}" == "ios" ] ; then
		for SUBPLATFORM in `find "lib/${PLATFORM}/" -mindepth 1 -maxdepth 1 -type d` ; do
			doPackage "${PLATFORM}" "Universal" $(basename "${SUBPLATFORM}")
		done
	else
		for ARCH in `find "lib/${PLATFORM}/" -mindepth 1 -maxdepth 1 -type d` ; do
			doPackage "${PLATFORM}" $(basename "${ARCH}")
		done
	fi	
done

# Package up the includes
OPENSSL_HDR_TAR="${PACKAGE_DIR}/OpenSSL-${OpenSSL_VERSION}-All-Universal-Headers.tar"
ICU_HDR_TAR="${PACKAGE_DIR}/ICU-${ICU_VERSION}-All-Universal-Headers.tar"
tar -cf "${OPENSSL_HDR_TAR}" include/openssl/*.h
tar -cf "${ICU_HDR_TAR}" include/layout/*.h include/unicode/*.h
bzip2 -z --best "${OPENSSL_HDR_TAR}"
bzip2 -z --best "${ICU_HDR_TAR}"

