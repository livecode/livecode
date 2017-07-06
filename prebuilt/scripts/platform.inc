# OSX options
DEVELOPER="/Applications/Xcode.app/Contents/Developer"
PLATFORM_DEVELOPER="${DEVELOPER}/Platforms/MacOSX.platform/Developer"
SDK="MacOSX10.9.sdk"
OSXGCCVERSION=4.2
OSXMIN="10.6"

# Generic options
export MAKEFLAGS="-j1"

# Needed for cross-compiles
case $(uname) in
	Linux*)
		HOST_PLATFORM=linux
		;;
	Darwin*)
		HOST_PLATFORM=mac
		;;
	CYGWIN*)
		HOST_PLATFORM=win32
		;;
esac

case $(uname -m) in
	x86_64)
		HOST_ARCH=x86_64
		;;
	i*86)
		HOST_ARCH=i386
		;;
esac

# Which platform are we actually targeting?
case "${PLATFORM}" in
	mac)
		export UNIVERSAL_ARCHS="i386 x86_64"
		;;
	emscripten)
		export EMMAKE="emmake"
		export EMCONFIGURE="emconfigure"
		;;
	ios)
		source "${BASEDIR}/scripts/ios.inc"
		if [ "${ARCH}" == "universal" ] ; then
			queryiOS "${SUBPLATFORM}"
			export UNIVERSAL_ARCHS="${ARCHS}"
		fi
		;;
esac

if [ "${PLATFORM}" == "android" ] ; then
	source "${BASEDIR}/scripts/android.inc"
fi

function setCCForTarget {
	local PLATFORM=$1
	local ARCH=$2
	local SUBPLATFORM=$3
	
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
			export CC_BASE="${DEVELOPER}/usr/bin/gcc --sysroot=${PLATFORM_DEVELOPER}/SDKs/${SDK} -isysroot ${PLATFORM_DEVELOPER}/SDKs/${SDK} -mmacosx-version-min=${OSXMIN}"
			export CXX_BASE="${DEVELOPER}/usr/bin/g++ --sysroot=${PLATFORM_DEVELOPER}/SDKs/${SDK} -isysroot ${PLATFORM_DEVELOPER}/SDKs/${SDK} -mmacosx-version-min=${OSXMIN}"
			export CC="${CC_BASE} -arch ${ARCH}"
			export CXX="${CXX_BASE} -arch ${ARCH}"
			;;
		linux)
			export CC_BASE="gcc"
			export CXX_BASE="g++"
			# When building with newer binutils (>= 2.26), new types of
			# relocations are added that older binutils don't understand.
			# To suppress this, pass '-Wa,-mrelax-relocations=no' to the
			# compiler.
			echo "*DEBUG* binutils version test"
			BINUTILS_VERSION=`as --version | head -n 1 | egrep -o '[[:digit:]]+(\.[[:digit:]]+)+.*'`
			echo "*DEBUG* binutils version: ${BINUTILS_VERSION}"
			if printf '%s\n%s\n' 2.26.1 "${BINUTILS_VERSION}" | sort -cV; then
				CC_FLAGS_X86_NORELAX="-Wa,-mrelax-relocations=no"
			else
				CC_FLAGS_X86_NORELAX=
			fi
			echo "*DEBUG* CC_FLAGS: ${CC_FLAGS_X86_NORELAX}"
			
			if [ "${ARCH}" == "x86_64" ] ; then
				export CC="${CC_BASE} -m64 ${CC_FLAGS_X86_NORELAX}"
				export CXX="${CXX_BASE} -m64 ${CC_FLAGS_X86_NORELAX}"
			else
				export CC="${CC_BASE} -m32 ${CC_FLAGS_X86_NORELAX}"
				export CXX="${CXX_BASE} -m32 ${CC_FLAGS_X86_NORELAX}"
			fi
			;;
		android)
			if [ "${ARCH}" == "armv6" ] ; then
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
			queryiOS "${SUBPLATFORM}"
			
			export CROSS_TOP="${XCODE}/Platforms/${SUBPLATFORM_NAME}.platform/Developer"
			export CROSS_SDK="${SUBPLATFORM_NAME}${VERSION}.sdk"
			
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
