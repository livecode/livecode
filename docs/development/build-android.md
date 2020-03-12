# Compiling LiveCode for Android

![LiveCode Community Logo](http://livecode.com/wp-content/uploads/2015/02/livecode-logo.png)

Copyright © 2015-2017 LiveCode Ltd., Edinburgh, UK

## Dependencies

### Host system

We recommend performing Android builds on a Linux system.  It is also possible to build for Android on Mac OS X.

The main non-standard dependency needed for building LiveCode is a Java Development Kit (JDK).  At least JDK 7 is required.

### Installing the Android SDK and NDK

LiveCode requires both the Android Software Development Kit (SDK) and Native Development Kit (NDK).  You can download both from the [Android Developers site](https://developer.android.com/sdk/index.html).

Extract both the NDK and SDK to a suitable directory, e.g. `~/android/toolchain`.  For example, for the following values of `<host>`

- mac: `darwin`
- linux: `linux`
- windows: `windows`

you would run:

````bash
mkdir -p ~/android/toolchain/android-sdk-<host>
cd ~/android/toolchain

wget https://dl.google.com/android/repository/android-ndk-r15-<host>-x86_64.zip
wget https://dl.google.com/android/repository/tools_r25.2.3-<host>.zip

unzip android-ndk-r15-<host>-x86_64.zip

pushd android-sdk-<host>
unzip ../tools_r25.2.3-<host>.zip
popd
````

Update the SDK:

    android-sdk-<host>/tools/android update sdk --no-ui

(This command will download and install every Android SDK and will take some time).

### Creating the toolchains

Create a standalone toolchain (this simplifies setting up the build environment):

````bash
for arch in arm arm64 x86 x86_64; do
    android-ndk-r15/build/tools/make_standalone_toolchain.py \
        --arch ${arch} --api 21 --deprecated-headers \
        --install-dir ${HOME}/android/toolchain/standalone-${arch}
done

**Note:** If you are only interested in building one particular flavour
of Android engines, leave out the architectures you're not interested in
from the above commands.

### Final toolchain setup

Add a couple of symlinks to allow the engine configuration script to find the Android toolchain:

````bash
ln -s android-ndk-r15 android-ndk
ln -s android-sdk-<host> android-sdk
````

## Configuring LiveCode

### Build environment

The Android build expects a number of environment variables to be set.  If the environment variables aren't set, the build process will attempt to guess sensible defaults. If you've set up the directory structure as described above, the make command should detect everything automatically and these variables shouldn't be necessary.

The following script will set up the environment variables correctly.  You may need to edit it depending on where your JDK and toolchain are installed:

````bash
ANDROID_TOOLCHAIN_DIR=${HOME}/android/toolchain # Edit me!

# Java SDK
JAVA_SDK=/Library/Java/JavaVirtualMachines/jdk1.8.0_131.jdk/Contents/Home

# Android platform information
ANDROID_NDK_VERSION=r15
ANDROID_NDK_PLATFORM_VERSION=16
ANDROID_API_VERSION=28
ANDROID_PLATFORM=android-${ANDROID_API_VERSION}
ANDROID_NDK=${ANDROID_TOOLCHAIN_DIR}/android-ndk-${ANDROID_NDK_VERSION}
ANDROID_SDK=${ANDROID_TOOLCHAIN_DIR}/android-sdk
ANDROID_BUILD_TOOLS=28.0.3

export JAVA_SDK ANDROID_TOOLCHAIN_DIR
export ANDROID_NDK_PLATFORM_VERSION ANDROID_API_VERSION
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

The Linux build environment used for compiling LiveCode for Android is based on Debian Jessie x86-64, with the following additional packages installed:

* git
* bzip2
* p7zip-full
* zip
* python
* build-essential
* openjdk-7-jdk
* flex
* bison
