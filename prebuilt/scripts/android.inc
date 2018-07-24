# Android options
ANDROID_NDK=${ANDROID_NDK:-"${HOME}/android/toolchain/android-ndk"}
ANDROID_PLATFORM=${ANDROID_PLATFORM:-android-26}

function configureAndroidToolchain {
	ANDROID_ARCH=$1

# Android architecture
if [ -z "${ANDROID_ARCH}" ] ; then
	echo >&2 "Error: \${ANDROID_ARCH} must be set"
	exit 1
fi

# Expected Android standalone toolchain directory name
case "${ANDROID_ARCH}" in
	armv6|armv7)
		ANDROID_STANDALONE_TOOLCHAIN_DIRNAME="standalone-arm"
		;;
	*)
		ANDROID_STANDALONE_TOOLCHAIN_DIRNAME="standalone-${ANDROID_ARCH}"
		;;
esac

# Expected Android target triple and architecture flag(s)
case "${ANDROID_ARCH}" in
	armv6|armv7)
		# Google recommends using Thumb instructions when targeting ARM.
		ANDROID_TRIPLE=arm-linux-androideabi
		if [ "${ANDROID_ARCH}" == "armv7" ] ; then
			BUILD_ARCH=armv7a
		else
			BUILD_ARCH=armv6
		fi
		ANDROID_TARGET_FLAGS="-target ${ANDROID_TRIPLE} -march=${BUILD_ARCH} -mthumb"
		;;
	arm64)
		ANDROID_TRIPLE=aarch64-linux-android
		ANDROID_TARGET_FLAGS="-target ${ANDROID_TRIPLE}"
		;;
	x86)
		ANDROID_TRIPLE=i686-linux-android
		ANDROID_TARGET_FLAGS="-target ${ANDROID_TRIPLE} -march=i686"
		;;
	x86_64)
		ANDROID_TRIPLE=x86_64-linux-android
		ANDROID_TARGET_FLAGS="-target ${ANDROID_TRIPLE}"
		;;
	*)
		ANDROID_TRIPLE=
		ANDROID_TARGET_FLAGS=
		;;
esac

# Has a full, custom toolchain been specified?
#  AR, CC, CXX, LINK, OBJCOPY, RANLIB, STRIP
if [ ! -z "${OBJCOPY}" ] ; then
	ANDROID_CC="${CC}"
	ANDROID_CXX="${CXX}"
	ANDROID_LINK="${LINK}"
else
	# Attempt to set up the toolchain automatically
	if [ -z "${ANDROID_TOOLCHAIN}" ] ; then
		ANDROID_TOOLCHAIN="${HOME}/android/toolchain/${ANDROID_STANDALONE_TOOLCHAIN_DIRNAME}/bin/${ANDROID_TRIPLE}-"
		# OpenSSL requires this to be set when cross-compiling for Android
		export CROSS_SYSROOT="${HOME}/android/toolchain/${ANDROID_STANDALONE_TOOLCHAIN_DIRNAME}/sysroot"
	else
		# if not defined, derive CROSS_ROOT from ANDROID_TOOLCHAIN
		if [ -z "${CROSS_SYSROOT}" ] ; then
			export CROSS_SYSROOT="${ANDROID_TOOLCHAIN%/*/*}/sysroot"
		fi
	fi

	ANDROID_CC_NAME=${ANDROID_CC_NAME:-clang}
	ANDROID_CXX_NAME=${ANDROID_CXX_NAME:-clang++}

	export ANDROID_CC="${ANDROID_TOOLCHAIN}${ANDROID_CC_NAME} ${ANDROID_TARGET_FLAGS}"
	export ANDROID_CXX="${ANDROID_TOOLCHAIN}${ANDROID_CXX_NAME} ${ANDROID_TARGET_FLAGS}"
	export ANDROID_LINK="${ANDROID_TOOLCHAIN}${ANDROID_CXX_NAME} ${ANDROID_TARGET_FLAGS}"
	export ANDROID_AR="${ANDROID_TOOLCHAIN}ar"
	export ANDROID_OBJCOPY="${ANDROID_TOOLCHAIN}objcopy"
	export ANDROID_RANLIB="${ANDROID_TOOLCHAIN}ranlib"
	export ANDROID_STRIP="${ANDROID_TOOLCHAIN}strip"

	# ICU needs to know the Android triple being targeted.
	export ANDROID_TRIPLE
fi

}

# Android architectures
export ARCHS="armv6 armv7 x86 x86_64"
