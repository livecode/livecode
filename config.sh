#!/bin/bash
# Copyright (C) 2015 LiveCode Ltd.
#
# This file is part of LiveCode.
#
# LiveCode is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License v3 as published by the Free
# Software Foundation.
#
# LiveCode is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
#
# You should have received a copy of the GNU General Public License
# along with LiveCode.  If not see <http://www.gnu.org/licenses/>.

usage () {
  cat <<EOF
Use gyp to generate project files when compiling LiveCode.

Usage:
  config.sh [--platform PLATFORM] [options ...] [gyp_options ...]

Options:
  -p, --platform PLATFORM
                    Choose which platform to build for
  -h, --help        Print this message

gyp Options:

    --generator-output DIR
                    Puts generated build files under DIR
    --depth PATH    Set DEPTH gyp variable to a relative path to PATH
    -f, format FORMATS
                    Output formats to generate
    -Gmsvs_version=WIN_MSVS_VERSION
                    Version of Microsoft Visual Studio to be used
    -Gandroid_ndk_version=ANDROID_NDK_VERSION
                    Version of Android Native Development Kit to be used
    -DOS=OS         Compile LiveCode for target operating system
    -Dtarget_arch=TARGET_ARCH
                    Compile LiveCode to run on ARCH processors
    -Dtarget_sdk=XCODE_TARGET_SDK
                    Compile LiveCode using the specified SDK in Xcode
    -Dhost_sdk=XCODE_HOST_SDK
                    Compile build tools using the specified SDK in Xcode

All unrecognised options get passed directly to gyp.  If you don't specify an
required option, config.sh will try to guess a suitable value.

The currently-supported PLATFORMs are:

  mac
  ios
  linux-x86
  linux-x86_64
  android-armv6
  win-x86
  emscripten

EOF
  exit $1
}

guess_platform () {
  case "$(uname -s)" in
    Darwin)
      echo "mac"
      return 0 ;;
    Linux)
      case "$(uname -p)" in
        x86_64)
          echo "linux-x86_64"
          return 0 ;;
        i*86|x86)
          echo "linux-x86"
          return 0 ;;
      esac;;
  esac
  return 1
}

invoke_gyp () {
  echo gyp/gyp "$@" 2>&1
  exec gyp/gyp "$@"
}

################################################################
# Parse command-line options
################################################################

num_save_opts=0

while [[ $# > $num_save_opts ]]; do

  key="$1"

  case "$key" in
    -h|--help)
      usage 1
      ;;
    -p|--platform)
      PLATFORM="$2"
      shift
      ;;
    --generator-output)
      GENERATOR_OUTPUT="$2"
      shift
      ;;
    --depth)
      DEPTH="$2"
      shift
      ;;
    -f|--format)
      FORMATS="$2 ${FORMATS}"
      shift
      ;;

    # Intercept -D options that config.sh tries to generate
    -D*)
      d="${key#*=}"
      case "$key" in
        -Dhost_sdk=*)    XCODE_HOST_SDK="$d" ;;
        -Dtarget_sdk=*)  XCODE_TARGET_SDK="$d" ;;
        -Dtarget_arch=*) TARGET_ARCH="$d" ;;
        -DOS=*)          OS="$d" ;;
        *) # Pass directly through to gyp
          set x "$@" "$key"
          let num_save_opts++
          shift
          ;;
      esac
      ;;

    # Intercept -G options that config.sh tries to generate
    -G*)
      d="${key#*=}"
      case "$key" in
        -Gmsvs_version=*)        WIN_MSVS_VERSION="$d" ;;
        -Gandroid_ndk_version=*) ANDROID_NDK_VERSION="$d" ;;

        *) # Pass directly through to gyp
          set x "$@" "$key"
          let num_save_opts++
          shift
          ;;
      esac
      ;;
    *) # Unrecognised option
      echo "ERROR: Unrecognised option '$key'"
      exit 1
      ;;
  esac

  shift
done

################################################################
# Guess and validate platform
################################################################

# If no platform specified, try to guess the platform
if test -z "${PLATFORM}"; then
  PLATFORM=$(guess_platform)
  if test $? -ne 0; then
    echo "ERROR: Cannot guess platform; specify '--platform <name>'" >&2
    exit 1
  fi
fi

# Validate platform
case ${PLATFORM} in
  linux-x86) ;;
  linux-x86_64) ;;
  android-armv6) ;;
  mac) ;;
  ios) ;;
  win-x86) ;;
  emscripten) ;;
  *)
    echo "ERROR: Unrecognised platform: '${PLATFORM}'" >&2
    exit 1;;
esac

################################################################
# Guess other relevant options
################################################################

# Guess generator output directory from platform
if test -z "$GENERATOR_OUTPUT"; then
  GENERATOR_OUTPUT="build-${PLATFORM}/livecode"
fi

# Guess OS from platform
if test -z "$OS"; then
  case ${PLATFORM} in
    linux*) OS="linux" ;;
    android*) OS="android" ;;
    mac*) OS="mac" ;;
    ios*) OS="ios" ;;
    win*) OS="win" ;;
    emscripten*) OS="emscripten" ;;
  esac
fi

# If no output type specified, guess from platform:
if test -z "$FORMATS"; then
  case ${OS} in
    # Always use Linux-style makefiles for Android as the Android toolchain
    # is more Linux-y than Darwin-y
    linux|android|emscripten) FORMATS="make-linux" ;;
    mac|ios)       FORMATS="xcode" ;;
    win)           FORMATS="msvs" ;;
  esac
fi

# Default "depth"
if test -z "$DEPTH"; then
  DEPTH=.
fi

# Default Visual Studio version
if test -z "$WIN_MSVS_VERSION"; then
  WIN_MSVS_VERSION=2010
fi

# Default Xcode target SDK
if test -z "$XCODE_TARGET_SDK"; then
 case ${OS} in
   mac) XCODE_TARGET_SDK="macosx10.9" ;;
   ios) XCODE_TARGET_SDK="iphoneos" ;;
 esac
fi

# Default Xcode host SDK
if test -z "$XCODE_HOST_SDK"; then
  case ${OS} in
    mac) XCODE_HOST_SDK="${XCODE_TARGET_SDK}" ;;
    ios) XCODE_HOST_SDK="macosx" ;;
  esac
fi

# Default target architectures
# iOS architectures are restricted to 32-bit only for iOS 5.1, 6.1 and 7.1
if test -z "$TARGET_ARCH"; then
  case ${PLATFORM} in
    *-x86)     TARGET_ARCH="x86" ;;
    *-x86_64)  TARGET_ARCH="x86_64" ;;
    *-armv6)   TARGET_ARCH="armv6" ;;
    emscripten) TARGET_ARCH="js" ;;

    mac*|ios*)
      case ${XCODE_TARGET_SDK} in
        macosx*)         		TARGET_ARCH="i386" ;;
        iphoneos5* | \
        iphoneos6* | \
        iphoneos7*)		 		TARGET_ARCH="armv7" ;;
        iphoneos*)       		TARGET_ARCH="armv7 arm64" ;;
        iphonesimulator5* | \
        iphonesimulator6* | \
        iphonesimulator7*) 		TARGET_ARCH="i386" ;;
        iphonesimulator*)  	TARGET_ARCH="i386 x86_64" ;;
      esac
      ;;

    *)
      echo "Couldn't guess 'target_arch'"
  esac
fi

# Location of Perl when running Windows builds
WIN_PERL=${WIN_PERL:-"C:/perl/bin/perl.exe"}

# Android default settings and tools
if test "${OS}" = "android" ; then
    ANDROID_NDK_VERSION=${ANDROID_NDK_VERSION:-r10d}
    ANDROID_PLATFORM=${ANDROID_PLATFORM:-android-17}

    # Attempt to locate an Android NDK
    if [ -z "${ANDROID_NDK}" -a "${OS}" = "android" ] ; then
        # Try the symlink we suggest in INSTALL-android.md
        if [ -d "${HOME}/android/toolchain/android-ndk" ] ; then
            ANDROID_NDK="${HOME}/android/toolchain/android-ndk"
        else
            echo >&2 "Error: Android NDK not found (set \$ANDROID_NDK)"
            exit 1
        fi
    fi

    # Attempt to locate an Android SDK
    if [ -z "${ANDROID_SDK}" ] ; then
        # Try the symlink we suggest in INSTALL-android.md
        if [ -d "${HOME}/android/toolchain/android-sdk" ] ; then
            ANDROID_SDK="${HOME}/android/toolchain/android-sdk"
        else
            echo >&2 "Error: Android SDK not found (set \$ANDROID_SDK)"
            exit 1
        fi
    fi

    # Attempt to guess the Android build tools version
    if [ -z "${ANDROID_BUILD_TOOLS}" ] ; then
        # Check for a sub-folder in the appropriate place
        # Possibly fragile - are there ever multiple sub-folders?
        if [ ! "$(echo \"${ANDROID_SDK}/build-tools/\"*)" = "${ANDROID_SDK}/build-tools/*" ] ; then
            ANDROID_BUILD_TOOLS=$(basename $(echo "${ANDROID_SDK}/build-tools/"*))
        else
            echo >&2 "Error: Android build tools not found (set \$ANDROID_BUILD_TOOLS)"
            exit 1
        fi
    fi

    if [ -z "${ANDROID_TOOLCHAIN}" ] ; then
        # Try the folder we suggest in INSTALL-android.md
        if [ -d "${HOME}/android/toolchain/standalone" ] ; then
            ANDROID_TOOLCHAIN="${HOME}/android/toolchain/standalone/bin/arm-linux-androideabi-"
        else
            echo >&2 "Error: Android toolchain not found (set \$ANDROID_TOOLCHAIN)"
            exit 1
        fi
    fi

    ANDROID_AR=${AR:-${ANDROID_TOOLCHAIN}ar}
    ANDROID_CC=${CC:-${ANDROID_TOOLCHAIN}clang -target arm-linux-androideabi -march=armv6 -integrated-as}
    ANDROID_CXX=${CXX:-${ANDROID_TOOLCHAIN}clang++ -target arm-linux-androideabi -march=armv6 -integrated-as}
    ANDROID_LINK=${LINK:-${ANDROID_TOOLCHAIN}clang++ -target arm-linux-androideabi -march=armv6 -integrated-as -fuse-ld=bfd}
    ANDROID_OBJCOPY=${OBJCOPY:-${ANDROID_TOOLCHAIN}objcopy}
    ANDROID_OBJDUMP=${OBJDUMP:-${ANDROID_TOOLCHAIN}objdump}
    ANDROID_STRIP=${STRIP:-${ANDROID_TOOLCHAIN}strip}

    if [ -z "${JAVA_SDK}" ] ; then
        # Utility used to locate Java on OSX systems
        if [ -x /usr/libexec/java_home ] ; then
            ANDROID_JAVA_SDK="$(/usr/libexec/java_home)"
        elif [ -d /usr/lib/jvm/default ] ; then
            ANDROID_JAVA_SDK=/usr/lib/jvm/default
        elif [ -d /usr/lib/jvm/default-java ] ; then
            ANDROID_JAVA_SDK=/usr/lib/jvm/default-java
        else
            echo >&2 "Error: no Java SDK found - set \$JAVA_SDK"
            exit 1
        fi
    else
        ANDROID_JAVA_SDK="${JAVA_SDK}"
    fi

fi # End of Android defaults & tools


# Emscripten default settings and tools
if test "${OS}" = "emscripten" ; then
    NODE_JS=${NODE_JS:-node}
fi


################################################################
# Invoke gyp
################################################################

format_args="$(for f in ${FORMATS}; do echo --format ${f} ; done)"

if test "${OS}" = "win" ; then
	basic_args="${format_args} --depth ${DEPTH} --generator-output ${GENERATOR_OUTPUT}"
else
	basic_args="${format_args} --depth ${DEPTH} --generator-output ${GENERATOR_OUTPUT} -G default_target=default"
fi

if [ "${BUILD_EDITION}" == "commercial" ] ; then
  basic_args="${basic_args} ../livecode-commercial.gyp"
fi

case ${OS} in
  linux)
    invoke_gyp $basic_args "-DOS=${OS}" "-Dtarget_arch=${TARGET_ARCH}" "$@"
    ;;
  emscripten)
    export NODE_JS
    invoke_gyp $basic_args "-DOS=${OS}" "-Dtarget_arch=${TARGET_ARCH}" "$@"
    ;;
  android)
    export ANDROID_BUILD_TOOLS
    export ANDROID_NDK
    export ANDROID_PLATFORM
    export ANDROID_SDK

    export JAVA_SDK="${ANDROID_JAVA_SDK}"

    export AR="${ANDROID_AR}"
    export CC="${ANDROID_CC}"
    export CXX="${ANDROID_CXX}"
    export LINK="${ANDROID_LINK}"
    export OBJCOPY="${ANDROID_OBJCOPY}"
    export OBJDUMP="${ANDROID_OBJDUMP}"
    export STRIP="${ANDROID_STRIP}"
    invoke_gyp $basic_args "-DOS=${OS}" "-Dtarget_arch=${TARGET_ARCH}" \
                           -Dcross_compile=1 \
                           "-Gandroid_ndk_version=${ANDROID_NDK_VERSION}" "$@"
    ;;
  win)
    invoke_gyp $basic_args "-Gmsvs_version=${WIN_MSVS_VERSION}" \
                           "-Dunix_configure=1" \
                           "-Dperl=${WIN_PERL}" "$@"
    ;;
  mac|ios)
    invoke_gyp $basic_args "-DOS=${OS}" \
                           "-Dtarget_sdk=${XCODE_TARGET_SDK}" \
                           "-Dhost_sdk=${XCODE_HOST_SDK}" \
                           "-Dtarget_arch=${TARGET_ARCH}" "$@"
    ;;
  *)
    echo "ERROR: Bad configuration for generating project files"
    exit 1 ;;
esac
