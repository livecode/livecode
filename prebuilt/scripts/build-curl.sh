#!/bin/bash

source "${BASEDIR}/scripts/platform.inc"
source "${BASEDIR}/scripts/lib_versions.inc"
source "${BASEDIR}/scripts/util.inc"

# Configuration flags
CURL_CONFIG="--disable-debug \
            --enable-http --enable-ftp --disable-file --disable-ldap --disable-ldaps --disable-rtsp --disable-dict --disable-telnet \
            --disable-tftp --disable-pop3 --disable-imap --disable-smtp \
            --disable-manual \
            --enable-shared=no \
            --disable-sspi --disable-crypto-auth --disable-cookies \
            --without-gnutls --without-polarssl --without-nss --without-libssh2 --without-librtmp --without-libidn"

# Grab the source for the library
CURL_TGZ="curl-${Curl_VERSION}.tar.gz"
CURL_SRC="curl-${Curl_VERSION}"
cd "${BUILDDIR}"

if [ ! -d "$CURL_SRC" ] ; then
	if [ ! -e "$CURL_TGZ" ] ; then
		echo "Fetching Curl source"
		fetchUrl "https://curl.haxx.se/download/curl-${Curl_VERSION}.tar.gz" "${CURL_TGZ}"
		if [ $? != 0 ] ; then
			echo "    failed"
			if [ -e "${CURL_TGZ}" ] ; then 
				rm ${CURL_TGZ} 
			fi
			exit
		fi
	fi
	
	echo "Unpacking Curl source"
	tar -xf "${CURL_TGZ}"
fi
				

function buildCurl {
	local PLATFORM=$1
	local ARCH=$2
	local SUBPLATFORM=$3

	CURL_ARCH_SRC="${CURL_SRC}-${PLATFORM}-${ARCH}"
	
	CURL_ARCH_CONFIG="${CURL_CONFIG} --prefix=${INSTALL_DIR}/${PLATFORM}/${ARCH} --with-ssl=${INSTALL_DIR}/${PLATFORM}/${ARCH}"
	
	if [ "${PLATFORM}" == "mac" -a "${ARCH}" == "ppc" ] ; then
		CURL_ARCH_CONFIG="${CURL_ARCH_CONFIG} --host i386"
	fi

	if [ ! -z "${CROSS_HOST}" ] ; then
		CURL_ARCH_CONFIG="${CURL_ARCH_CONFIG} --host ${CROSS_HOST}"
	fi
	
	if [ ! -d "${CURL_ARCH_SRC}" ] ; then
		echo "Duplicating Curl source directory for ${PLATFORM}/${ARCH}"
		cp -r "${CURL_SRC}" "${CURL_ARCH_SRC}"
	fi
	
	if [ -e "${CURL_ARCH_SRC}/config.cmd" ] ; then
		CURL_ARCH_CURRENT_CONFIG=`cat ${CURL_ARCH_SRC}/config.cmd`
	else
		CURL_ARCH_CURRENT_CONFIG=
	fi
	
	# Re-configure and re-build, if required
	if [ "${CURL_ARCH_CONFIG}" != "${CURL_ARCH_CURRENT_CONFIG}" ] ; then
		cd "${CURL_ARCH_SRC}"
		echo "Configuring and building Curl for ${PLATFORM}/${ARCH}"
		setCCForTarget "${PLATFORM}" "${ARCH}" "${SUBPLATFORM}"
		
		if [ "${PLATFORM}" == "linux" ] ; then
			export LDFLAGS="-Wl,-rpath,.,-rpath-link,${INSTALL_DIR}/${PLATFORM}/${ARCH}/lib"
		fi
		
		./configure ${CURL_ARCH_CONFIG} && make clean && make ${MAKEFLAGS} && make install
		RESULT=$?
		cd ..
		
		if [ $RESULT == 0 ] ; then
			echo "${CURL_ARCH_CONFIG}" > "${CURL_ARCH_SRC}/config.cmd"
		else
			echo "    failed"
			exit 1
		fi
	else
		echo "Found existing Curl build for ${PLATFORM}/${ARCH}"
	fi
	
	CURL_LIBS+="${INSTALL_DIR}/${PLATFORM}/${ARCH}/lib/libcurl.a "
	
	if [ ! "${ARCH}" == "universal" ] ; then
		mkdir -p "${OUTPUT_DIR}/lib/${PLATFORM}/${ARCH}"
		cp "${INSTALL_DIR}/${PLATFORM}/${ARCH}/lib/libcurl.a" "${OUTPUT_DIR}/lib/${PLATFORM}/${ARCH}/libcurl.a"
	fi
}

if [ "${ARCH}" == "universal" ] ; then
	# perform build for universal architectures
	for UARCH in ${UNIVERSAL_ARCHS} ; do
		buildCurl "${PLATFORM}" "${UARCH}" "${SUBPLATFORM}"
	done

	# Create the universal libraries
	echo "Creating Curl universal libraries"
	mkdir -p "${OUTPUT_DIR}/lib/${PLATFORM}/${SUBPLATFORM}"
	lipo -create ${CURL_LIBS} -output "${OUTPUT_DIR}/lib/${PLATFORM}/${SUBPLATFORM}/libcurl.a"
	CURL_LIBS=
else
	buildCurl "${PLATFORM}" "${ARCH}" "${SUBPLATFORM}"
fi

	
