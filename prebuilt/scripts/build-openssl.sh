#!/bin/bash

set -e

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
	
	# Boolean flag: if non-zero then configure CC/LD/CFLAGS/LDFLAGS etc.
	local CONFIGURE_CC_FOR_TARGET=1

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
			elif [[ "${ARCH}" =~ (x|i[3-6])86 ]] ; then
				SPEC="linux-x86"
			elif [ "${ARCH}" == "arm64" ] ; then
				SPEC="linux-aarch64"
			elif [[ "${ARCH}" =~ .*64 ]] ; then
				SPEC="linux-generic64"
			else
				SPEC="linux-generic32"
			fi
			;;
		android)
			configureAndroidToolchain "${ARCH}"
			export ANDROID_NDK_HOME="${ANDROID_TOOLCHAIN_BASE}"
			export PATH="${ANDROID_NDK_HOME}/bin:${PATH}"
			CONFIGURE_CC_FOR_TARGET=0

			if [ "${ARCH}" == "x86_64" ] ; then
				SPEC="android-x86_64"
			elif [[ "${ARCH}" =~ (x|i[3-6])86 ]] ; then
				# Work around a linker crash using the i686 gold linker in Android NDK r14
				export CFLAGS="-fuse-ld=bfd"
				SPEC="android-x86"
			elif [ "${ARCH}" == "arm64" ] ; then
				# Clang's integrated assembler is a bit broken so we need to force the use of GAS instead
				export CFLAGS="-fno-integrated-as"
				SPEC="android-arm64"
			elif [[ "${ARCH}" =~ armv(6|7) ]] ; then
				# Clang's integrated assembler is a bit broken so we need to force the use of GAS instead
				export CFLAGS="-fno-integrated-as"

				# When compiling with -mthumb, we need to link to libatomic
				EXTRA_OPTIONS="-latomic"

				SPEC="android-arm"
			elif [[ "${ARCH}" =~ .*64 ]] ; then
				SPEC="android64"
			else
				SPEC="android"
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
	
	# The android-* targets derive the arch from the last portion of the target name
	# so this needs to be a prefix instead of suffix.
	CUSTOM_SPEC="livecode_${SPEC}"

	OPENSSL_ARCH_SRC="${OPENSSL_SRC}-${PLATFORM_NAME}-${ARCH}"
	OPENSSL_ARCH_CONFIG="no-rc5 no-hw no-threads shared -DOPENSSL_NO_ASYNC=1 --prefix=${INSTALL_DIR}/${NAME} ${CUSTOM_SPEC} ${EXTRA_OPTIONS}"

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
my %targets = (
"${CUSTOM_SPEC}" => {
	inherit_from => [ "${SPEC}" ],
	bn_ops => add("EXPORT_VAR_AS_FN"),
},
);
EOF

		if [ $CONFIGURE_CC_FOR_TARGET != 0 ] ; then
			setCCForTarget "${PLATFORM}" "${ARCH}" "${SUBPLATFORM}"
		fi

		echo "Configuring OpenSSL for ${NAME}"
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
