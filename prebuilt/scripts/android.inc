# Android options
ANDROID_NDK=${ANDROID_NDK:-"${HOME}/android/toolchain/android-ndk"}
ANDROID_PLATFORM=${ANDROID_PLATFORM:-android-26}

# Has a full, custom toolchain been specified?
#  AR, CC, CXX, LINK, OBJCOPY, RANLIB, STRIP
if [ ! -z "${OBJCOPY}" ] ; then
	ANDROID_CC="${CC}"
	ANDROID_CXX="${CXX}"
	ANDROID_LINK="${LINK}"
else
	# Attempt to set up the toolchain automatically
	if [ -z "${ANDROID_TOOLCHAIN}" ] ; then
		ANDROID_TOOLCHAIN="${HOME}/android/toolchain/standalone/bin/arm-linux-androideabi-"
		# OpenSSL requires this to be set when cross-compiling for Android
		export CROSS_SYSROOT="${HOME}/android/toolchain/standalone/sysroot"
	else
		# if not defined, derive CROSS_ROOT from ANDROID_TOOLCHAIN
		if [ -z "${CROSS_SYSROOT}" ] ; then
			export CROSS_SYSROOT="${ANDROID_TOOLCHAIN%/*/*}/sysroot"
		fi
	fi

	ANDROID_CC_NAME=${ANDROID_CC_NAME:-gcc}
	ANDROID_CXX_NAME=${ANDROID_CXX_NAME:-g++}

	ANDROID_CC="${ANDROID_TOOLCHAIN}${ANDROID_CC_NAME}"
	ANDROID_CXX="${ANDROID_TOOLCHAIN}${ANDROID_CXX_NAME}"
	ANDROID_LINK="${ANDROID_TOOLCHAIN}${ANDROID_CXX_NAME}"
	ANDROID_AR="${ANDROID_TOOLCHAIN}ar"
	ANDROID_OBJCOPY="${ANDROID_TOOLCHAIN}objcopy"
	ANDROID_RANLIB="${ANDROID_TOOLCHAIN}ranlib"
	ANDROID_STRIP="${ANDROID_TOOLCHAIN}strip"
fi

# Android architectures
export ARCHS="armv6"
