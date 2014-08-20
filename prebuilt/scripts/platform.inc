# OSX options
DEVELOPER="${HOME}/Builds/Platforms/Xcode_3_2_6"
SDK="MacOSX10.5.sdk"
OSXGCCVERSION=4.2
OSXMIN="10.5"

# Linux options
LINUXGCCPATH=/usr/local/gcc_4-2-4/bin
LINUXGCCVERSION=4.2.4

# Android options
ANDROID_NDK="$HOME/Workspace/livecode/sdks/android-ndk"
ANDROID_TOOLCHAIN="${ANDROID_NDK}/toolchains/arm-linux-androideabi-4.4.3/prebuilt/darwin-x86/bin/arm-linux-androideabi"
ANDROID_PLATFORM="${ANDROID_NDK}/platforms/android-8"

# Generic options
export MAKEFLAGS="-j1"

# Was a platform specified?
if [ -z "${PLATFORM}" ] ; then
	case $(uname) in
		Darwin|Darwin64)
			export PLATFORM="mac"
			;;
		*)
			export PLATFORM="linux"
			;;
	esac
fi

# Which platform are we actually targeting?
case "${PLATFORM}" in
	mac)
		export ARCHS="i386 x86_64"
		export CC_BASE="${DEVELOPER}/usr/bin/gcc-${OSXGCCVERSION} --sysroot=${DEVELOPER}/SDKs/${SDK} -isysroot ${DEVELOPER}/SDKs/${SDK} -mmacosx-version-min=${OSXMIN}"
		export CXX_BASE="${DEVELOPER}/usr/bin/g++-${OSXGCCVERSION} --sysroot=${DEVELOPER}/SDKs/${SDK} -isysroot ${DEVELOPER}/SDKs/${SDK} -mmacosx-version-min=${OSXMIN}"
		;;
	linux)
		export ARCHS="i386"
		export CC_BASE="${LINUXGCCPATH}/gcc-${LINUXGCCVERSION}"
		export CXX_BASE="${LINUXGCCPATH}/g++-${LINUXGCCVERSION}"
		;;
	android)
		export ARCHS="armv6 armv7-a"
		export CC_BASE="${ANDROID_TOOLCHAIN}-gcc --sysroot ${ANDROID_PLATFORM}/arch-arm -fpic -ffunction-sections -funwind-tables -fno-strict-aliasing -finline-limit=64"
		export CXX_BASE="${ANDROID_TOOLCHAIN}-g++ --sysroot ${ANDROID_PLATFORM}/arch-arm -fpic -ffunction-sections -funwind-tables -fno-strict-aliasing -finline-limit=64 -frtti -fexceptions"
		export LD="${ANDROID_TOOLCHAIN}-ld"
		export AR="${ANDROID_TOOLCHAIN}-ar"
		export RANLIB="${ANDROID_TOOLCHAIN}-ranlib"
		export STRIP="${ANDROID_TOOLCHAIN}-strip"
		export ANDROID_DEV="${ANDROID_PLATFORM}/arch-arm/usr"
esac

# Allow the arch to be set manually
if [ ! -z "${CUSTOM_ARCH}" ] ; then
	export ARCHS="${CUSTOM_ARCH}"
fi

if [ "${PLATFORM}" == "ios" ] ; then
	source "${BASEDIR}/scripts/ios.inc"
fi

function setArchs {
	local SUBPLATFORM_INDEX=$1
	
	export ARCHS=${IOS_ARCHS[$SUBPLATFORM_INDEX]}
}

function setCCForArch {
	local ARCH=$1
	local SUBPLATFORM_INDEX=$2
	
	# Use the custom CC and CXX, if set
	if [ ! -z "${CUSTOM_CC}" ] ; then
		export CC="${CUSTOM_CC}"
		export CXX="${CUSTOM_CXX}"
		return
	fi
	
	case "${PLATFORM}" in
		mac)
			export CC="${CC_BASE} -arch $1"
			export CXX="${CXX_BASE} -arch $1"
			;;
		linux)
			if [ "$1" == "x86_64" ] ; then
				export CC="${CC_BASE} -m64"
				export CXX="${CXX_BASE} -m64"
			else
				export CC="${CC_BASE} -m32"
				export CXX="${CXX_BASE} -m32"
			fi
			;;
		android)
			if [ "$1" == "armv6 " ] ; then
				export CC="${CC_BASE} -mthumb ${ANDROID_CFLAGS}"
				export CXX="${CXX_BASE} -mthumb ${ANDROID_CFLAGS}"
				export LDFLAGS="${ANDROID_LDFLAGS}"
			else
				export CC="${CC_BASE} -march=armv7-a -mfloat-abi=softfp -mfpu=vfpv3-d16 ${ANDROID_CFLAGS}"
				export CXX="${CXX_BASE} -march=armv7-a -mfloat-abi=softfp -mfpu=vfpv3-d16 ${ANDROID_CFLAGS}"
				export LDFLAGS="-march=armv7-a -Wl,--fix-cortex-a8 ${ANDROID_LDFLAGS}"
			fi
			export LINK="${CXX}"
			;;
		ios)
			XCODE=${IOS_XCODE[$SUBPLATFORM_INDEX]}
			SUBPLATFORM=${IOS_SUBPLATFORM[$SUBPLATFORM_INDEX]}
			VERSION=${IOS_VERSION[$SUBPLATFORM_INDEX]}
			
			export CROSS_TOP="${XCODE}/Platforms/${SUBPLATFORM}.platform/Developer"
			export CROSS_SDK="${SUBPLATFORM}${VERSION}.sdk"
			
			if [ "${ARCH}" == "armv7" -o "${ARCH}" == "armv7s" ] ; then
				ARCH_FLAGS="-mthumb"
			else
				ARCH_FLAGS=
			fi
			
			if [ -x "${CROSS_TOP}/usr/bin/gcc" ] ; then
				CCPATH="${CROSS_TOP}/usr/bin"
			else
				CCPATH="${XCODE}/usr/bin"
			fi
			
			export CC="${CCPATH}/gcc -arch ${ARCH} ${ARCH_FLAGS} -isysroot ${CROSS_TOP}/SDKs/${CROSS_SDK} -miphoneos-version-min=4.3"
			export CXX="${CCPATH}/g++ -arch ${ARCH} ${ARCH_FLAGS} -isysroot ${CROSS_TOP}/SDKs/${CROSS_SDK} -miphoneos-version-min=4.3"
			export LDFLAGS="-L${CROSS_TOP}/SDKs/${CROSS_SDK}/usr/lib -isysroot ${CROSS_TOP}/SDKs/${CROSS_SDK} -Wl,-dead_strip"
			;;
	esac
}
