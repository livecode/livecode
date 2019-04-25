#!/bin/bash
#Abort packaging on error
set -e

# Versions
source "scripts/platform.inc"
source "scripts/lib_versions.inc"

# Package directory
PACKAGE_DIR="`pwd`/packaged"
mkdir -p "${PACKAGE_DIR}"

function generateTarFileName {
	local LIBNAME=$1
	local PLATFORM=$2
	
	eval local BUILDREVISION="\$${LIBNAME}_BUILDREVISION"
	eval local VERSION="\$${LIBNAME}_VERSION"
	
	# Tar file name may include prebuilt build revision number
	if [ ! -z "${BUILDREVISION}" ] ; then
		local TAR_FILE="${PACKAGE_DIR}/${LIBNAME}-${VERSION}-${PLATFORM}-${BUILDREVISION}.tar"
	else
		local TAR_FILE="${PACKAGE_DIR}/${LIBNAME}-${VERSION}-${PLATFORM}.tar"
	fi
	
	eval ${LIBNAME}_TAR='"${TAR_FILE}"'
}

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
	
	if [ "${PLATFORM}" == "ios" ] ; then
		source "scripts/ios.inc"
		queryiOS "${SUBPLATFORM}"
		local PACKAGE_SUBPLATFORM="${SUBPLATFORM_NAME}${VERSION}"
	elif  [ "${PLATFORM}" == "android" ] ; then
		local PACKAGE_SUBPLATFORM="${SUBPLATFORM}"
	else
		local PACKAGE_SUBPLATFORM=
	fi

	echo "Creating packages: ${PLATFORM} ${ARCH} ${SUBPLATFORM}"

	if [ "${ARCH}" == "universal" ] ; then
		local ARCHDIR=
	else
		local ARCHDIR="${ARCH}"
	fi

	if [ ! -z "${PACKAGE_SUBPLATFORM}" ] ; then
		local SUFFIX="${PLATFORM}-${PACKAGE_ARCH}-${PACKAGE_SUBPLATFORM}"
	else
		local SUFFIX="${PLATFORM}-${PACKAGE_ARCH}"
	fi

	local LIBPATH="lib/${PLATFORM}/${ARCHDIR}/${SUBPLATFORM}"
	local SHAREPATH="share/${PLATFORM}/${ARCHDIR}/${SUBPLATFORM}"
	local BINPATH="bin/${PLATFORM}/${ARCHDIR}/${SUBPLATFORM}"

	generateTarFileName OpenSSL "${SUFFIX}"
	generateTarFileName Curl "${SUFFIX}"
	generateTarFileName ICU "${SUFFIX}"
	generateTarFileName CEF "${SUFFIX}"
	generateTarFileName Thirdparty "${SUFFIX}"
	
	# Package up OpenSSL
	if [ -f "${LIBPATH}/libcustomcrypto.a" ] ; then
		tar -cf "${OpenSSL_TAR}" "${LIBPATH}/libcustomcrypto.a" "${LIBPATH}/libcustomssl.a"
	elif [ -f "${LIBPATH}/revsecurity.dll" ] ; then
		tar -cf "${OpenSSL_TAR}" "${LIBPATH}/libeay32.lib" "${LIBPATH}/ssleay32.lib" "${LIBPATH}/revsecurity.dll" "${LIBPATH}/revsecurity.def"
	fi

	# Package up Curl
	if [ -f "${LIBPATH}/libcurl.a" ] ; then
		tar -cf "${Curl_TAR}" "${LIBPATH}/libcurl.a"
	elif [ -f "${LIBPATH}/libcurl_a.lib" ] ; then
		tar -cf "${Curl_TAR}" "${LIBPATH}/libcurl_a.lib"
	fi

	# Package up ICU
	local ICU_FILES=
	if [ -f "${LIBPATH}/libicudata.a" ] ; then
		for LIB in data i18n io le lx tu uc ; do
			if [ -f "${LIBPATH}/libicu${LIB}.a" ] ; then
				ICU_FILES+="${LIBPATH}/libicu${LIB}.a "
			fi
		done
	elif [ -f "${LIBPATH}/sicudt.lib" ] ; then
		for LIB in dt in io le lx tu uc ; do
			if [ -f "${LIBPATH}/sicu${LIB}.lib" ] ; then
				ICU_FILES+="${LIBPATH}/sicu${LIB}.lib "
			fi
		done
	fi
	if [ -f "${BINPATH}/pkgdata" ] ; then
		ICU_FILES+="${BINPATH}/pkgdata "
	fi
	if [ -f "${BINPATH}/icupkg" ] ; then
		ICU_FILES+="${BINPATH}/icupkg "
	fi
	if [ ! -z "${ICU_FILES}" ] ; then
		tar -cf "${ICU_TAR}" ${ICU_FILES}
	fi

	# Package up CEF
	if [ "${PLATFORM}" == "linux" ] ; then
		if [ -d "${LIBPATH}/CEF" ] ; then
			tar -cf "${CEF_TAR}" "${LIBPATH}/CEF"
		fi
	fi

	# Package up Thirdparty
	local Thirdparty_FILES=
	local Thirdparty_LIBS_I="Thirdparty_LIBS_$PLATFORM"
	if [ -f "${LIBPATH}/libz.a" ] ; then
		for LIB in ${!Thirdparty_LIBS_I} ; do
			Thirdparty_FILES+="${LIBPATH}/${LIB}.a "
			if [ -e "${LIBPATH}/${LIB}_opt_none.a" ]; then
				Thirdparty_FILES+="${LIBPATH}/${LIB}_*.a "
			fi
		done
	else
		for LIB in ${!Thirdparty_LIBS_I} ; do
			Thirdparty_FILES+="${LIBPATH}/${LIB}.lib "
			if [ "${LIB}" == "libskia" ]; then
				Thirdparty_FILES+="${LIBPATH}/${LIB}_*.lib "
			fi
		done
	fi

	if [ ! -z "${Thirdparty_FILES}" ] ; then
		tar -cf "${Thirdparty_TAR}" ${Thirdparty_FILES}
	fi

	# Compress the packages
	if [ -f "${OpenSSL_TAR}" ] ; then
		bzip2 -zf --best "${OpenSSL_TAR}"
	fi
	if [ -f "${Curl_TAR}" ] ; then
		bzip2 -zf --best "${Curl_TAR}"
	fi
	if [ -f "${ICU_TAR}" ] ; then
		bzip2 -zf --best "${ICU_TAR}"
	fi
	if [ -f "${CEF_TAR}" ] ; then
		bzip2 -zf --best "${CEF_TAR}"
	fi
	if [ -f "${Thirdparty_TAR}" ] ; then
		bzip2 -zf --best "${Thirdparty_TAR}"
	fi
}

PLATFORM=$1
ARCH=$2

#only ios and android subplatforms are used
if [ "${PLATFORM}" == "ios" ] || [ "${PLATFORM}" == "android" ] ; then
	SUBPLATFORM=$3
else
	SUBPLATFORM=
fi

# Package up the various libraries and headers
doPackage "${PLATFORM}" "${ARCH}" "${SUBPLATFORM}"

# We only need shared headers to be packaged once, so only do this on linux-x86_64
if [ "${PLATFORM}" = "linux" -a "${ARCH}" = "x86_64" ] ; then
	# Package up the includes
	OPENSSL_HDR_NAME="OpenSSL-${OpenSSL_VERSION}-All-Universal-Headers"
	if [ ! -z "${OpenSSL_BUILDREVISION}" ] ; then
		OPENSSL_HDR_NAME+="-${OpenSSL_BUILDREVISION}"
	fi
	ICU_HDR_NAME="ICU-${ICU_VERSION}-All-Universal-Headers"
	ICU_DATA_NAME="ICU-${ICU_VERSION}-All-Universal-Data"
	if [ ! -z "${ICU_BUILDREVISION}" ] ; then
		ICU_HDR_NAME+="-${ICU_BUILDREVISION}"
		ICU_DATA_NAME+="-${ICU_BUILDREVISION}"
	fi
	tar -cf "${PACKAGE_DIR}/${OPENSSL_HDR_NAME}.tar" include/openssl/*.h
	tar -cf "${PACKAGE_DIR}/${ICU_HDR_NAME}.tar" include/unicode/*.h
	tar -cf "${PACKAGE_DIR}/${ICU_DATA_NAME}.tar" share/icu*.dat
	bzip2 -zf --best "${PACKAGE_DIR}/${OPENSSL_HDR_NAME}.tar"
	bzip2 -zf --best "${PACKAGE_DIR}/${ICU_HDR_NAME}.tar"
	bzip2 -zf --best "${PACKAGE_DIR}/${ICU_DATA_NAME}.tar"
fi
