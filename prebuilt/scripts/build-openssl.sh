#!/bin/bash

source "${BASEDIR}/scripts/platform.inc"
source "${BASEDIR}/scripts/lib_versions.inc"
source "${BASEDIR}/scripts/util.inc"

# Grab the source for the library
OPENSSL_TGZ="openssl-${OpenSSL_VERSION}.tar.gz"
OPENSSL_SRC="openssl-${OpenSSL_VERSION}"
cd "${BUILDDIR}"

if [ ! -d "$OPENSSL_SRC" ] ; then
	if [ ! -e "$OPENSSL_TGZ" ] ; then
		echo "Fetching OpenSSL source"
		fetchUrl "https://www.openssl.org/source/openssl-${OpenSSL_VERSION}.tar.gz" "${OPENSSL_TGZ}"
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
	local PLATFORM=$1
	local ARCH=$2
	local SUBPLATFORM=$3
	
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
	if [ ! -z "${SUBPLATFORM}" ] ; then
		local NAME="${PLATFORM}/${ARCH}/${SUBPLATFORM}"
		local PLATFORM_NAME=${SUBPLATFORM}
	else
		local NAME="${PLATFORM}/${ARCH}"
		local PLATFORM_NAME=${PLATFORM}
	fi
	
	CUSTOM_SPEC="${SPEC}-livecode"

	OPENSSL_ARCH_SRC="${OPENSSL_SRC}-${PLATFORM_NAME}-${ARCH}"
	OPENSSL_ARCH_CONFIG="no-rc5 no-hw shared -DOPENSSL_NO_ASYNC=1 --prefix=${INSTALL_DIR}/${NAME} ${CUSTOM_SPEC}"

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

		# Customise the OpenSSL configuration to ensure variables are exported as functions
		cat > Configurations/99-livecode.conf << EOF
%targets = (
"${CUSTOM_SPEC}" => {
	inherit_from => [ "${SPEC}" ],
	bn_ops => sub { join(" ",(@_,"EXPORT_VAR_AS_FN")) },
},
);
EOF

		echo "Configuring OpenSSL for ${NAME}"
		
		setCCForTarget "${PLATFORM}" "${ARCH}" "${SUBPLATFORM}"
		./Configure ${OPENSSL_ARCH_CONFIG}
		
		# iOS requires some tweaks to the source when building for devices
		if [ "${PLATFORM}" == "ios" -a "${ARCH}" != "i386 " ] ; then
			sed -i "" -e "s!static volatile sig_atomic_t intr_signal;!static volatile intr_signal;!" "crypto/ui/ui_openssl.c"
		fi
		
		# iOS SDKs don't work with makedepend
		if [ "${PLATFORM}" == "ios" ] ; then
			sed -i "" -e "s/MAKEDEPPROG=makedepend/MAKEDEPPROG=$\(CC\) -M/" Makefile
		fi

		echo "Building OpenSSL for ${NAME}"
		make clean && make depend && make ${MAKEFLAGS} && make install_sw
		RESULT=$?
		cd ..
		
		# Save the configuration for this build
		if [ $RESULT == 0 ] ; then
			echo "${OPENSSL_ARCH_CONFIG}" > "${OPENSSL_ARCH_SRC}/config.cmd"
		else
			echo "    failed"
			exit 1
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

	mkdir -p "${OUTPUT_DIR}/include"
	cp -R "${INSTALL_DIR}/${NAME}/include/openssl" "${OUTPUT_DIR}/include/openssl"
}


if [ "${ARCH}" == "universal" ] ; then
	# perform build for universal architectures
	for UARCH in ${UNIVERSAL_ARCHS} ; do
		buildOpenSSL "${PLATFORM}" "${UARCH}" "${SUBPLATFORM}"
	done

	# Create the universal libraries
	echo "Creating OpenSSL ${PLATFORM_NAME} universal libraries"
	mkdir -p "${OUTPUT_DIR}/lib/${PLATFORM}/${SUBPLATFORM}"
	lipo -create ${CRYPTO_LIBS} -output "${OUTPUT_DIR}/lib/${PLATFORM}/${SUBPLATFORM}/libcustomcrypto.a"
	lipo -create ${SSL_LIBS} -output "${OUTPUT_DIR}/lib/${PLATFORM}/${SUBPLATFORM}/libcustomssl.a"
	CRYPTO_LIBS=
	SSL_LIBS=
else
	buildOpenSSL "${PLATFORM}" "${ARCH}" "${SUBPLATFORM}"
fi
