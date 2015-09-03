# OSX options
DEVELOPER="${HOME}/Builds/Platforms/Xcode_3_2_6"
SDK="MacOSX10.5.sdk"
OSXGCCVERSION=4.2
OSXMIN="10.5"

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
		export ARCHS="i386 x86_64"
		export CC_BASE="clang"
		export CXX_BASE="clang++"
		;;
	emscripten)
		export ARCHS="js"
		export EMMAKE="emmake"
		export EMCONFIGURE="emconfigure"
		;;
esac

# Allow the arch to be set manually
if [ ! -z "${CUSTOM_ARCH}" ] ; then
	export ARCHS="${CUSTOM_ARCH}"
fi

if [ "${PLATFORM}" == "ios" ] ; then
	source "${BASEDIR}/scripts/ios.inc"
fi

if [ "${PLATFORM}" == "android" ] ; then
	source "${BASEDIR}/scripts/android.inc"
fi

function setArchs {
	local SUBPLATFORM_INDEX=$1
	
	export ARCHS=${IOS_ARCHS[$SUBPLATFORM_INDEX]}
}

function setCCForArch {
	local ARCH=$1
	local SUBPLATFORM_INDEX=$2
	
	# Use the custom CC and CXX, if set
	if [ ! -z "${CUSTOM_CC}" -a "${PLATFORM}" != "android" ] ; then
		export CC="${CUSTOM_CC}"
		export CXX="${CUSTOM_CXX}"
		return
	fi
	
	case "${PLATFORM}" in
		win32)
			export CFLAGS="/MT"
			export CXXFLAGS="/MT"
			;;
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
			if [ "$1" == "armv6" ] ; then
				export CC="${ANDROID_CC} ${ANDROID_CFLAGS}"
				export CXX="${ANDROID_CXX} ${ANDROID_CXXFLAGS}"
			else
				export CC="${ANDROID_CC} -march=armv7-a -mfloat-abi=softfp -mfpu=vfpv3-d16 ${ANDROID_CFLAGS}"
				export CXX="${ANDROID_CXX} -march=armv7-a -mfloat-abi=softfp -mfpu=vfpv3-d16 ${ANDROID_CXXFLAGS}"
				EXTRA_LDFLAGS="-march=armv7-a -Wl,--fix-cortex-a8"
			fi
			export LINK="${ANDROID_LINK} ${EXTRA_LDFLAGS} ${ANDROID_LDFLAGS}"
			export LD="${LINK}"
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
			
			export CC="${CCPATH}/gcc -arch ${ARCH} ${ARCH_FLAGS} -isysroot ${CROSS_TOP}/SDKs/${CROSS_SDK} -miphoneos-version-min=6.1"
			export CXX="${CCPATH}/g++ -arch ${ARCH} ${ARCH_FLAGS} -isysroot ${CROSS_TOP}/SDKs/${CROSS_SDK} -miphoneos-version-min=6.1"
			export LDFLAGS="-L${CROSS_TOP}/SDKs/${CROSS_SDK}/usr/lib -isysroot ${CROSS_TOP}/SDKs/${CROSS_SDK} -Wl,-dead_strip"
			;;
	esac
}
