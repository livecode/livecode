# Android options
ANDROID_NDK=${ANDROID_NDK:-"$HOME/Workspace/livecode/sdks/android-ndk"}
ANDROID_PLATFORM=${ANDROID_PLATFORM:-android-16}

# Has a full, custom toolchain been specified?
#  AR, CC, CXX, LINK, OBJCOPY, RANLIB, STRIP
if [ ! -z "${OBJCOPY}" ] ; then
	ANDROID_CC="${CC}"
	ANDROID_CXX="${CXX}"
	ANDROID_LINK="${LINK}"
	ANDROID_AR="${AR}"
	ANDROID_OBJCOPY="${OBJCOPY}"
	ANDROID_RANLIB="${RANLIB}"
	ANDROID_STRIP="${STRIP}"
else
	# Attempt to set up the toolchain automatically
	if [ -z "${ANDROID_TOOLCHAIN}" ] ; then
		ANDROID_TOOLCHAIN_armv6="${HOME}/android/toolchain/standalone-arm-linux-androideabi/bin/arm-linux-androideabi"
		ANDROID_TOOLCHAIN_armv7a="${ANDROID_TOOLCHAIN_armv6}"
		ANDROID_TOOLCHAIN_arm64_v8a="${HOME}/android/toolchain/standalone-aarch64-linux-android/bin/aarch64-linux-android"
		ANDROID_TOOLCHAIN_mipsel="${HOME}/android/toolchain/standalone-mipsel-linux-android/bin/mipsel-linux-android"
		ANDROID_TOOLCHAIN_mips64el="${HOME}/android/toolchain/standalone-mips64el-linux-android/bin/mips64el-linux-android"
		ANDROID_TOOLCHAIN_x86="${HOME}/android/toolchain/standalone-x86/bin/i686-linux-android"
		ANDROID_TOOLCHAIN_x86_64="${HOME}/android/toolchain/standalone-x86_64/bin/x86_64-linux-android"
	fi
	
	ANDROID_CFLAGS_armv6="-march=armv6"
	ANDROID_CFLAGS_armv7a="-march=armv7a -mthumb -mfpu=vfpv3-d16 -mfloat-abi=softfp"
	ANDROID_CFLAGS_mipsel=""
	ANDROID_CFLAGS_x86="-march=i686 -mtune=intel -mssse3 -mfpmath=sse -m32"
fi

# Android architectures
# All Android variants are buildable but not all are used 
if [ -z "${CUSTOM_ARCH}" ] ; then
	export ARCHS="armv6 armv7a arm64_v8a mipsel mips64el x86 x86_64"
else
	export ARCHS="${CUSTOM_ARCH}"
fi
