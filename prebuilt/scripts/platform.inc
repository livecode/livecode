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

if [ -z "${XCODEBUILD}" ] ; then
	export XCODEBUILD="/Applications/Xcode.app/Contents/Developer/usr/bin/xcodebuild"
	XCODE_DEVELOPER="/Applications/Xcode.app/Contents/Developer"
else
	# XCODEBUILD points to $XCODE_APP/Contents/Developer/usr/bin/xcodebuild
	XCODE_DEVELOPER="${XCODEBUILD%/*/*/*}"
fi

# Which platform are we actually targeting?
case "${PLATFORM}" in
	mac)
		export UNIVERSAL_ARCHS="i386 x86_64"
		;;
	linux)
		export ARCHS="i386 x86_64"
		export CC_BASE="${CUSTOM_CC:-gcc}"
		export CXX_BASE="${CUSTOM_CXX:-g++}"
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
	
	# Unset environment variables that may interfere cross-platform
	CFLAGS=
	CXXFLAGS=
	CC=
	CXX=
	LINK=
	LD=
	LDFLAGS=
	AR=
	OBJCOPY=
	RANLIB=
	STRIP=
	CROSS_TOP=
	CROSS_SDK=
	EMMAKE=
	EMCONFIGURE=
	
	case "${PLATFORM}" in
		win32)
			CFLAGS="/MT"
			CXXFLAGS="/MT"
			;;
		mac)
			# OSX options
			PLATFORM_DEVELOPER="${XCODE_DEVELOPER}/Platforms/MacOSX.platform/Developer"
			SDK="MacOSX10.9.sdk"
			OSXGCCVERSION=4.2
			OSXMIN="10.6"

			CC_BASE="${XCODE_DEVELOPER}/usr/bin/gcc --sysroot=${PLATFORM_DEVELOPER}/SDKs/${SDK} -isysroot ${PLATFORM_DEVELOPER}/SDKs/${SDK} -mmacosx-version-min=${OSXMIN}"
			CXX_BASE="${XCODE_DEVELOPER}/usr/bin/g++ --sysroot=${PLATFORM_DEVELOPER}/SDKs/${SDK} -isysroot ${PLATFORM_DEVELOPER}/SDKs/${SDK} -mmacosx-version-min=${OSXMIN}"
			CC="${CC_BASE} -arch ${ARCH}"
			CXX="${CXX_BASE} -arch ${ARCH}"
			;;
		linux)
			CC_BASE="gcc"
			CXX_BASE="g++"
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
			
			CC_FLAGS="-fPIC ${CC_FLAGS_X86_NORELAX}"
			
			echo "*DEBUG* CC_FLAGS: ${CC_FLAGS}"
			
			if [ "${ARCH}" == "x86_64" ] ; then
				CC="${CC_BASE} -m64 ${CC_FLAGS}"
				CXX="${CXX_BASE} -m64 ${CC_FLAGS}"
			elif [ "$1" == "x86" ] ; then
				CC="${CC_BASE} -m32 ${CC_FLAGS}"
				CXX="${CXX_BASE} -m32 ${CC_FLAGS}"
			fi
			;;
		android)
			configureAndroidToolchain ${ARCH}
			# if not defined, derive CROSS_ROOT from ANDROID_TOOLCHAIN_BASE
			if [ -z "${CROSS_SYSROOT}" ] ; then
				export CROSS_SYSROOT="${ANDROID_TOOLCHAIN_BASE}/sysroot"
			fi

			CC="${ANDROID_CC}"
			CFLAGS="${ANDROID_CFLAGS} ${EXTRA_CFLAGS}"
			CXX="${ANDROID_CXX}"
			CXXFLAGS="${ANDROID_CXXFLAGS} ${EXTRA_CXXFLAGS}"
			LINK="${ANDROID_LINK}"
			LD="${LINK}"
			LDFLAGS="${ANDROID_LDFLAGS}"
			AR="${ANDROID_AR}"
			OBJCOPY="${ANDROID_OBJCOPY}"
			RANLIB="${ANDROID_RANLIB}"
			STRIP="${ANDROID_STRIP}"
			;;
		ios)
			queryiOS "${SUBPLATFORM}"
			
			CROSS_TOP="${XCODE}/Platforms/${SUBPLATFORM_NAME}.platform/Developer"
			CROSS_SDK="${SUBPLATFORM_NAME}${VERSION}.sdk"
			
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
			
			CC="${CCPATH}/gcc -arch ${ARCH} ${ARCH_FLAGS} -isysroot ${CROSS_TOP}/SDKs/${CROSS_SDK} -miphoneos-version-min=7.0"
			CXX="${CCPATH}/g++ -arch ${ARCH} ${ARCH_FLAGS} -isysroot ${CROSS_TOP}/SDKs/${CROSS_SDK} -miphoneos-version-min=7.0"
			LDFLAGS="-L${CROSS_TOP}/SDKs/${CROSS_SDK}/usr/lib -isysroot ${CROSS_TOP}/SDKs/${CROSS_SDK} -Wl,-dead_strip"
			;;
		emscripten)
			echo "CUSTOM_EMMAKE: ${CUSTOM_EMMAKE}"
			echo "CUSTOM_EMCONFIGURE: ${CUSTOM_EMCONFIGURE}"
			if [ ! -z "${CUSTOM_EMMAKE}" ] ; then
				EMMAKE="${CUSTOM_EMMAKE}"
				if [ ! -z "${CUSTOM_EMCONFIGURE}" ] ; then
					EMCONFIGURE="${CUSTOM_EMCONFIGURE}"
				else
					# if EMMAKE is set but EMCONFIGURE is not then use folder of EMMAKE to find EMCONFIGURE
					EMCONFIGURE="${CUSTOM_EMMAKE%/*}/emconfigure"
				fi
			else
				EMMAKE="emmake"
				EMCONFIGURE="emconfigure"
			fi
			
			echo "EMMAKE: ${EMMAKE}"
			echo "EMCONFIGURE: ${EMCONFIGURE}"
			;;
	esac

	# Export compilation vars
	export CFLAGS
	export CXXFLAGS
	export CC
	export CXX
	export LINK
	export LD
	export LDFLAGS
	export AR
	export OBJCOPY
	export RANLIB
	export STRIP
	export CROSS_TOP
	export CROSS_SDK
	export EMMAKE
	export EMCONFIGURE
}
