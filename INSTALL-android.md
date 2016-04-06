# Compiling LiveCode for Android

![LiveCode Community Logo](http://livecode.com/wp-content/uploads/2015/02/livecode-logo.png)

Copyright © 2015 LiveCode Ltd., Edinburgh, UK

## Dependencies

### Host system

We recommend performing Android builds on a Linux system.  It is also possible to build for Android on Mac OS X.

The main non-standard dependency needed for building LiveCode is a Java Development Kit (JDK).  At least JDK 7 is required.

### Installing the Android SDK and NDK

LiveCode requires both the Android Software Development Kit (SDK) and Native Development Kit (NDK).  You can download both from the [Android Developers site](https://developer.android.com/sdk/index.html).

Extract both the NDK and SDK to a suitable directory, e.g. `~/android/toolchain`.  For example, you would run:

````bash
mkdir -p ~/android/toolchain
cd ~/android/toolchain

tar -xf ~/Downloads/android-sdk_r24.1.2-linux.tgz
7z x ~/Downloads/android-ndk-r10e-linux-x86_64.bin
````

Update the SDK:

    android-sdk-linux/tools/android update sdk --no-ui

### Setting up the ARM toolchain

Create a standalone toolchain (this simplifies setting up the build environment):

````bash
android-ndk-r10e/build/tools/make-standalone-toolchain.sh \
    --toolchain=arm-linux-androideabi-clang3.5 \
    --platform=android-10 \
    --install-dir=${HOME}/android/toolchain/standalone
````

Add a couple of symlinks to allow the engine configuration script to find the Android toolchain:

````bash
ln -s android-ndk-r10e android-ndk
ln -s android-sdk-linux android-sdk
````

## Configuring LiveCode

### Build environment

The Android build expects a large number of environment variables to be set.  If the environment variables aren't set, the build process will attempt to guess sensible defaults. If you've set up the directory structure as described above, the make command should detect everything automatically and these variables shouldn't be necessary.

The following script will set up the environment variables correctly.  You may need to edit it depending on where your JDK and ARM toolchain are installed:

````bash
ARCH=armv6
TRIPLE=arm-linux-androideabi

TOOLCHAIN=${HOME}/android/toolchain # Edit me!

# Java SDK
JAVA_SDK=/usr/lib/jvm/java-7-openjdk-amd64/ # Edit me!

# Build tools
BINDIR=$TOOLCHAIN/standalone/bin
COMMON_FLAGS="-target ${TRIPLE} -march=${ARCH}"

CC="${BINDIR}/${TRIPLE}-clang ${COMMON_FLAGS} -integrated-as"
CXX="${BINDIR}/${TRIPLE}-clang ${COMMON_FLAGS} -integrated-as"
LINK="${BINDIR}/${TRIPLE}-clang ${COMMON_FLAGS} -fuse-ld=bfd"
AR="${BINDIR}/${TRIPLE}-ar"

# Android platform information
ANDROID_NDK_VERSION=r10e
ANDROID_PLATFORM=android-10
ANDROID_NDK=${TOOLCHAIN}/android-ndk-r10e
ANDROID_SDK=${TOOLCHAIN}/android-sdk-linux
ANDROID_BUILD_TOOLS=23.0.1

export JAVA_SDK
export CC CXX LINK AR
export ANDROID_PLATFORM ANDROID_NDK ANDROID_SDK ANDROID_BUILD_TOOLS
````

### Generating makefiles

The gyp-based build system generates a set of makefiles which are used to control the build process.

To generate makefiles in a `build-android-armv6`, simply run:

    make config-android

To provide detailed configuration options, you can use the `config.sh` script.  For more information, run:

    ./config.sh --help

## Compiling LiveCode

Normally, it'll be enough just to use the top-level makefile:

    make -k compile-android

Otherwise, you'll need to build a target in the gyp-generated makefiles:

    make -C build-android-armv6/livecode development

## Standard build environment

**Note:** The following information is provided for reference purposes.  It should be possible to build LiveCode for Android on any modern Linux desktop distribution or recent version of Mac OS.

The Linux build environment used for compiling LiveCode for Android is based on Debian Squeeze x86-64, with the following additional packages installed:

* git
* bzip2
* p7zip-full
* zip
* python
* build-essential
* openjdk-7-jdk
