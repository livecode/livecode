#!/bin/bash

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

if [ "$SUBPLATFORM" == "" ]; then
	TARGET_PAIR=$PLATFORM
else
	TARGET_PAIR=$PLATFORM-$SUBPLATFORM
fi

if [ "$PLATFORM" == "mac" ]; then
	LIBS="${Thirdparty_LIBS_mac}"
	BUILDPATH="../_build/mac/Release"
	LIBPATH="lib/mac"
elif [ "$PLATFORM" == "linux" ]; then
	LIBS="${Thirdparty_LIBS_linux}"
	BUILDPATH=""
	LIBPATH="lib/linux"
elif [ "$PLATFORM" == "ios" ]; then
	LIBS="${Thirdparty_LIBS_ios}"
	BUILDPATH="../_build/ios/$SUBPLATFORM/Release"
	LIBPATH="lib/ios/$SUBPLATFORM"
elif [ "$PLATFORM" == "emscripten" ]; then
	LIBS="${Thirdparty_LIBS_emscripten}"
	BUILDPATH="../build-emscripten/livecode/out/Release/obj.target/thirdparty"
	LIBPATH="lib/emscripten/js"
elif [ "$PLATFORM" == "android" ]; then
	LIBS="${Thirdparty_LIBS_android}"
	BUILDPATH="../build-android-$ARCH/livecode/out/Release/obj.target/thirdparty"
	LIBPATH="lib/android/$ARCH"
elif [ "$PLATFORM" == "win32" ]; then
	LIBS="${Thirdparty_LIBS_win32}"
	BUILDPATH=""
	LIBPATH=""
fi

make -C .. config-$TARGET_PAIR

if [ "$PLATFORM" == "mac" ] || [ "$PLATFORM" == "ios" ] ; then
	${XCODEBUILD} -project "../build-$TARGET_PAIR/livecode/livecode.xcodeproj" -configuration "Release" -target "prebuilts"
	for t_lib in $LIBS ; do
		cp "$BUILDPATH/${t_lib}.a" "$LIBPATH"
		if [ "${t_lib}" == "libskia" ]; then
			cp "$BUILDPATH/${t_lib}"_*.a "$LIBPATH"
		fi
	done
elif [ "$PLATFORM" == "emscripten" ] ; then
	export BUILDTYPE=Release
	${EMMAKE} make -j16 -C ../build-$PLATFORM/livecode prebuilts
	for t_lib in $LIBS ; do
		cp "$BUILDPATH/${t_lib}/${t_lib}.a" "$LIBPATH"
	done
elif [ "$PLATFORM" == "android" ] ; then
	export BUILDTYPE=Release
	${EMMAKE} make -j16 -C ../build-$PLATFORM-$ARCH/livecode prebuilts
	for t_lib in $LIBS ; do
		cp "$BUILDPATH/${t_lib}/${t_lib}.a" "$LIBPATH"
	done
fi

