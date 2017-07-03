#!/bin/bash

source "${BASEDIR}/scripts/platform.inc"
source "${BASEDIR}/scripts/lib_versions.inc"

# Only for desktop platforms
if [ "${PLATFORM}" == "ios" -o "${PLATFORM}" == "android" ] ; then
	echo "Curl not required for platform ${PLATFORM}"
	exit 0
fi

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
		curl https://curl.haxx.se/download/curl-${Curl_VERSION}.tar.gz -o ${CURL_TGZ}
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
				

for ARCH in ${ARCHS}
do
	CURL_ARCH_SRC="${CURL_SRC}-${PLATFORM}-${ARCH}"
	
	CURL_ARCH_CONFIG="${CURL_CONFIG} --prefix=${INSTALL_DIR}/${PLATFORM}/${ARCH} --with-ssl=${INSTALL_DIR}/${PLATFORM}/${ARCH}"
	
	if [ "${PLATFORM}" == "mac" -a "${ARCH}" == "ppc" ] ; then
		CURL_ARCH_CONFIG="${CURL_ARCH_CONFIG} --host i386"
	fi

	if [ "${PLATFORM}" == "linux" -a "${ARCH}" == "armv6-hf" ] ; then
		CURL_ARCH_CONFIG="${CURL_ARCH_CONFIG} --host i386"
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
		setCCForArch "${ARCH}"
		
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
			exit
		fi
	else
		echo "Found existing Curl build for ${PLATFORM}/${ARCH}"
	fi
	
	CURL_LIBS+="${INSTALL_DIR}/${PLATFORM}/${ARCH}/lib/libcurl.a "
	
	if [ ! "${PLATFORM}" == "mac" ] ; then
		mkdir -p "${OUTPUT_DIR}/lib/${PLATFORM}/${ARCH}"
		cp "${INSTALL_DIR}/${PLATFORM}/${ARCH}/lib/libcurl.a" "${OUTPUT_DIR}/lib/${PLATFORM}/${ARCH}/libcurl.a"
	fi
done

# Create the universal libraries
if [ "${PLATFORM}" == "mac" ] ; then
	echo "Creating Curl mac universal libraries"
	mkdir -p "${OUTPUT_DIR}/lib/mac"
	lipo -create ${CURL_LIBS} -output "${OUTPUT_DIR}/lib/mac/libcurl.a"
fi
	
