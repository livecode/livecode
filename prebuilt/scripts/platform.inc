# OSX options
DEVELOPER="/Applications/Xcode.app/Contents/Developer"
PLATFORM_DEVELOPER="${DEVELOPER}/Platforms/MacOSX.platform/Developer"
SDK="MacOSX10.9.sdk"
OSXGCCVERSION=4.2
OSXMIN="10.6"

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
		export CC_BASE="${DEVELOPER}/usr/bin/gcc --sysroot=${PLATFORM_DEVELOPER}/SDKs/${SDK} -isysroot ${PLATFORM_DEVELOPER}/SDKs/${SDK} -mmacosx-version-min=${OSXMIN}"
		export CXX_BASE="${DEVELOPER}/usr/bin/g++ --sysroot=${PLATFORM_DEVELOPER}/SDKs/${SDK} -isysroot ${PLATFORM_DEVELOPER}/SDKs/${SDK} -mmacosx-version-min=${OSXMIN}"
		;;
	linux)
		export ARCHS="i386 x86_64"
		export CC_BASE="gcc"
		export CXX_BASE="g++"
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
			# When building with newer binutils (>= 2.26), new types of
			# relocations are added that older binutils don't understand.
			# To suppress this, pass '-Wa,-mrelax-relocations=no' to the
			# compiler.
			echo "*DEBUG* binutils version test"
			BINUTILS_VERSION=`ld --version | head -n 1 | egrep -o "[[:digit:]]+\.[[:digit:]]+\.[[:digit:]]+"`
			echo "*DEBUG* binutils version: ${BINUTILS_VERSION}"
			if printf '%s\n%s\n' 2.26.1 "${BINUTILS_VERSION}" | sort -cV; then
				CC_FLAGS_X86_NORELAX="-Wa,-mrelax-relocations=no"
			else
				CC_FLAGS_X86_NORELAX=
			fi
			echo "*DEBUG* CC_FLAGS: ${CC_FLAGS_X86_NORELAX}"
			
			if [ "$1" == "x86_64" ] ; then
				export CC="${CC_BASE} -m64 ${CC_FLAGS_X86_NORELAX}"
				export CXX="${CXX_BASE} -m64 ${CC_FLAGS_X86_NORELAX}"
			else
				export CC="${CC_BASE} -m32 ${CC_FLAGS_X86_NORELAX}"
				export CXX="${CXX_BASE} -m32 ${CC_FLAGS_X86_NORELAX}"
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
