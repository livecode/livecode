#!/bin/bash

source "${BASEDIR}/scripts/platform.inc"
source "${BASEDIR}/scripts/lib_versions.inc"

echo $Thirdparty_BUILDREVISION
echo $Thirdparty_VERSION

echo $PLATFORM
echo $SUBPLATFORM
echo $ARCH

if [ "$XCODEBUILD" == "" ]; then
	XCODEBUILD="xcodebuild"
fi

if [ "$EMMAKE" == "" ]; then
	EMMAKE="emmake"
fi

TARGET_NAME=$PLATFORM

if [ "$ARCH" != "" -a "$ARCH" != "universal" ]; then
	TARGET_NAME=$TARGET_NAME-$ARCH
fi

if [ "$SUBPLATFORM" != "" -a "$PLATFORM" != "android" ]; then
	TARGET_NAME=$TARGET_NAME-$SUBPLATFORM
fi

MAKE_TARGET=$TARGET_NAME

if [ "$PLATFORM" == "mac" ]; then
	LIBS="${Thirdparty_LIBS_mac}"
	BUILDPATH="../_build/mac/Release"
	LIBPATH="lib/mac"
elif [ "$PLATFORM" == "linux" ]; then
	LIBS="${Thirdparty_LIBS_linux}"
	BUILDPATH="../build-linux-$ARCH/livecode/out/Release/obj.target/thirdparty"
	LIBPATH="lib/linux/$ARCH"
elif [ "$PLATFORM" == "ios" ]; then
	LIBS="${Thirdparty_LIBS_ios}"
	BUILDPATH="../_build/ios/$SUBPLATFORM/Release"
	LIBPATH="lib/ios/$SUBPLATFORM"
elif [ "$PLATFORM" == "emscripten" ]; then
	MAKE_TARGET=emscripten
	LIBS="${Thirdparty_LIBS_emscripten}"
	BUILDPATH="../build-emscripten/livecode/out/Release/obj.target/thirdparty"
	LIBPATH="lib/emscripten/js"
elif [ "$PLATFORM" == "android" ]; then
	LIBS="${Thirdparty_LIBS_android}"
	BUILDPATH="../build-android-$ARCH/livecode/out/Release/obj.target/thirdparty"
	LIBPATH="lib/android/$ARCH/$SUBPLATFORM"
elif [ "$PLATFORM" == "win32" ]; then
	LIBS="${Thirdparty_LIBS_win32}"
	BUILDPATH=""
	LIBPATH=""
fi

make -C .. config-$MAKE_TARGET

if [ "$PLATFORM" == "mac" ] || [ "$PLATFORM" == "ios" ] ; then
	${XCODEBUILD} -project "../build-$TARGET_NAME/livecode/livecode.xcodeproj" -configuration "Release" -target "thirdparty-prebuilts"
elif [ "$PLATFORM" == "linux" ] ; then
	export BUILDTYPE=Release
	make -C "../build-${PLATFORM}-${ARCH}/livecode" thirdparty-prebuilts
elif [ "$PLATFORM" == "emscripten" ] ; then
	export BUILDTYPE=Release
	${EMMAKE} make -j16 -C "../build-${PLATFORM}/livecode" thirdparty-prebuilts
elif [ "$PLATFORM" == "android" ] ; then
	export BUILDTYPE=Release
	make -j16 -C "../build-${PLATFORM}-${ARCH}/livecode" thirdparty-prebuilts
fi

mkdir -p "$LIBPATH"

for t_lib in $LIBS ; do
	if [ "$PLATFORM" == "mac" ] || [ "$PLATFORM" == "ios" ] ; then
		cp "$BUILDPATH/${t_lib}.a" "$LIBPATH"
		if [ "${t_lib}" == "libskia" ]; then
			cp "$BUILDPATH/${t_lib}"_*.a "$LIBPATH"
		fi
	elif [ "$PLATFORM" == "linux" ] || [ "$PLATFORM" == "android" ] || [ "$PLATFORM" == "emscripten" ]; then
		cp "$BUILDPATH/${t_lib}/${t_lib}.a" "$LIBPATH"
		if [ "${t_lib}" == "libskia" ]; then
			cp "$BUILDPATH/${t_lib}/${t_lib}"_*.a "$LIBPATH"
		fi
	fi
done
