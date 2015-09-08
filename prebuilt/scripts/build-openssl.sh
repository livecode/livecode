#!/bin/bash

source "${BASEDIR}/scripts/platform.inc"
source "${BASEDIR}/scripts/lib_versions.inc"

# Grab the source for the library
OPENSSL_TGZ="openssl-${OpenSSL_VERSION}.tar.gz"
OPENSSL_SRC="openssl-${OpenSSL_VERSION}"
cd "${BUILDDIR}"

if [ ! -d "$OPENSSL_SRC" ] ; then
	if [ ! -e "$OPENSSL_TGZ" ] ; then
		echo "Fetching OpenSSL source"
		curl http://www.openssl.org/source/openssl-${OpenSSL_VERSION}.tar.gz -o "${OPENSSL_TGZ}"
		if [ $? != 0 ] ; then
			echo "    failed"
			if [ -e "${OPENSSL_TGZ}" ] ; then 
				rm ${OPENSSL_TGZ} 
			fi
			exit
		fi
	fi
	
	echo "Unpacking OpenSSL source"
	tar -xf "${OPENSSL_TGZ}"
fi



function buildOpenSSL {
	local SUBPLATFORM_INDEX=$1
	
	for ARCH in ${ARCHS} 
	do
		# Each target type in OpenSSL is given a name
		case "${PLATFORM}" in
			mac)
				if [ "${ARCH}" == "x86_64" -o "${ARCH}" == "ppc64" ] ; then
					SPEC="darwin64-${ARCH}-cc"
				else
					SPEC="darwin-${ARCH}-cc"
				fi
				;;
			linux)
        if [ "${ARCH}" == "x86_64" ] ; then
          SPEC="linux-x86_64"
				else
		      SPEC="linux-generic32"
				fi
				;;
			android)
				if [ "${ARCH}" == "armv6" ] ; then
					SPEC="android"
				else
					SPEC="android-armv7"
				fi
				;;
			ios)
				if [ "${ARCH}" == "x86_64" ] ; then
					SPEC="darwin64-x86_64-cc"
				else
					SPEC="iphoneos-cross"
				fi
				;;
		esac
	
		# Utility for displaying platform name
		if [ ! -z "${SUBPLATFORM_INDEX}" ] ; then
			local SUBPLATFORM="${IOS_SUBPLATFORM[$SUBPLATFORM_INDEX]}${IOS_VERSION[$SUBPLATFORM_INDEX]}"
			local NAME="${PLATFORM}/${SUBPLATFORM}/${ARCH}"
			local PLATFORM_NAME=${SUBPLATFORM}
		else
			local NAME="${PLATFORM}/${ARCH}"
			local PLATFORM_NAME=${PLATFORM}
		fi
		
		OPENSSL_ARCH_SRC="${OPENSSL_SRC}-${PLATFORM_NAME}-${ARCH}"
		OPENSSL_ARCH_CONFIG="no-idea no-rc5 no-hw shared --prefix=${INSTALL_DIR}/${NAME} ${SPEC}"
		OPENSSL_ARCH_LOG="${OPENSSL_ARCH_SRC}.log"
	
		# Copy the source to a target-specific directory
		if [ ! -d "${OPENSSL_ARCH_SRC}" ] ; then
			echo "Duplicating OpenSSL source directory for ${NAME}"
			cp -r "${OPENSSL_SRC}" "${OPENSSL_ARCH_SRC}"
		fi
	
		# Get the command used to build a previous copy, if any
		if [ -e "${OPENSSL_ARCH_SRC}/config.cmd" ] ; then
			OPENSSL_ARCH_CURRENT_CONFIG=`cat ${OPENSSL_ARCH_SRC}/config.cmd`
		else
			OPENSSL_ARCH_CURRENT_CONFIG=
		fi
	
		# Re-configure and re-build, if required
		if [ "${OPENSSL_ARCH_CONFIG}" != "${OPENSSL_ARCH_CURRENT_CONFIG}" ] ; then
			cd "${OPENSSL_ARCH_SRC}"
			echo "Configuring OpenSSL for ${NAME}"
			
			setCCForArch "${ARCH}" "${SUBPLATFORM_INDEX}"
			./Configure ${OPENSSL_ARCH_CONFIG} > "${OPENSSL_ARCH_LOG}" 2>&1
			
			# iOS requires some tweaks to the source when building for devices
			if [ "${PLATFORM}" == "ios" -a "${ARCH}" != "i386 " ] ; then
				sed -i "" -e "s!static volatile sig_atomic_t intr_signal;!static volatile intr_signal;!" "crypto/ui/ui_openssl.c"
			fi
			
			# Ensure that variables get exported as functions
			echo "#define OPENSSL_EXPORT_VAR_AS_FUNCTION 1" >> crypto/opensslconf.h

			echo "Building OpenSSL for ${NAME}"
			make clean >> "${OPENSSL_ARCH_LOG}" 2>&1 && make ${MAKEFLAGS} >> "${OPENSSL_ARCH_LOG}" 2>&1 && make install_sw >> "${OPENSSL_ARCH_LOG}" 2>&1
			RESULT=$?
			cd ..
			
			# Save the configuration for this build
			if [ $RESULT == 0 ] ; then
				echo "${OPENSSL_ARCH_CONFIG}" > "${OPENSSL_ARCH_SRC}/config.cmd"
			else
				echo "    failed"
				exit
			fi
		else
			echo "Found existing OpenSSL build for ${NAME}"
		fi
	
		if [ "${PLATFORM}" == "mac" -o "${PLATFORM}" == "ios" ] ; then
			CRYPTO_LIBS+="${INSTALL_DIR}/${NAME}/lib/libcrypto.a "
			SSL_LIBS+="${INSTALL_DIR}/${NAME}/lib/libssl.a "
		else
			mkdir -p "${OUTPUT_DIR}/lib/${NAME}"
			cp "${INSTALL_DIR}/${NAME}/lib/libcrypto.a" "${OUTPUT_DIR}/lib/${NAME}/libcustomcrypto.a"
			cp "${INSTALL_DIR}/${NAME}/lib/libssl.a" "${OUTPUT_DIR}/lib/${NAME}/libcustomssl.a"
		fi
	done

	# Create the universal libraries
	if [ "${PLATFORM}" == "mac" -o "${PLATFORM}" == "ios" ] ; then
		echo "Creating OpenSSL ${PLATFORM_NAME} universal libraries"
		mkdir -p "${OUTPUT_DIR}/lib/${PLATFORM}/${SUBPLATFORM}"
		lipo -create ${CRYPTO_LIBS} -output "${OUTPUT_DIR}/lib/${PLATFORM}/${SUBPLATFORM}/libcustomcrypto.a"
		lipo -create ${SSL_LIBS} -output "${OUTPUT_DIR}/lib/${PLATFORM}/${SUBPLATFORM}/libcustomssl.a"
		CRYPTO_LIBS=
		SSL_LIBS=
	fi
}



# If building for iOS, a number of sub-platforms need to be build
if [ "${PLATFORM}" == "ios" ] ; then
	for (( INDEX=0; INDEX<$IOS_COUNT; INDEX++ ))
	do
		setArchs ${INDEX}
		buildOpenSSL ${INDEX}
	done
else
	buildOpenSSL
fi

