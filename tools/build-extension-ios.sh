#!/bin/bash

set -e

SYMBOLS=$1
SYMBOLS_FILE=$2
COPY_PATH=$3

DEPS=`cat "$SRCROOT/$PRODUCT_NAME.ios"`
DEPS=${DEPS//library /-l}
DEPS=${DEPS//framework /-framework }

if [ "$SYMBOLS" != "" ]; then
	SYMBOLS="-Wl,-exported_symbol -Wl,${SYMBOLS// / -Wl,-exported_symbol -Wl,}"
fi

if [ -e "$SYMBOLS_FILE" ]; then
	SYMBOLS+="-Wl,-exported_symbols_list $SYMBOLS_FILE"
fi

if [ "$STATIC_DEPS" != "" ]; then
	DEPS=$STATIC_DEPS
fi

if [ -e $PLATFORM_DEVELOPER_BIN_DIR/g++ ]; then
	BIN_DIR=$PLATFORM_DEVELOPER_BIN_DIR
else
	BIN_DIR=$DEVELOPER_BIN_DIR
fi

# MW-2011-09-19: Updated to build universal binary version of lcext - by passing
#   the process through g++ we get it all for free!
# MW-2013-06-26: [[ CloneAndRun ]] When in 'Debug' mode, don't strip all global symbols.
if [ $CONFIGURATION = "Debug" ]; then
	STRIP_OPTIONS="-Wl,-r"
else
	STRIP_OPTIONS="-Wl,-r -Wl,-x"
fi

# SN-2015-02019: [[ Bug 14625 ]] We build and link each arch separately, and lipo them
#  togother once it's done.

# We still want to produce dylibs for the simulator
if [ "$EFFECTIVE_PLATFORM_NAME" = "-iphonesimulator" ]; then
	BUILD_DYLIB=1
else
	BUILD_DYLIB=0
fi

# SN-2015-02-19: [[ Bug 14625 ]] Xcode only create FAT headers from iOS SDK 7.0
FAT_INFO=$(otool -fv "$BUILT_PRODUCTS_DIR/$EXECUTABLE_NAME" | grep "Fat headers" || true)

if [ -z "$FAT_INFO" -o $BUILD_DYLIB -eq 1 ]; then
	# We set the minimum iOS or simulator version
    if [ $BUILD_DYLIB -eq 1 ]; then
        MIN_OS_VERSION="-mios-simulator-version-min=6.0.0"
    else
        MIN_OS_VERSION="-miphoneos-version-min=6.0.0"
    fi

    ARCHS="-arch ${ARCHS// / -arch }"

	if [ $BUILD_DYLIB -eq 1 ]; then
		$BIN_DIR/g++ -stdlib=libc++ -dynamiclib ${ARCHS} $MIN_OS_VERSION -isysroot $SDKROOT -L"$SOLUTION_DIR/prebuilt/lib/ios/$SDK_NAME" -o "$BUILT_PRODUCTS_DIR/$PRODUCT_NAME.dylib" "$BUILT_PRODUCTS_DIR/$EXECUTABLE_NAME" -dead_strip -Wl,-x $SYMBOLS $DEPS
	fi

	if [ $? -ne 0 ]; then
		exit $?
	fi

	$BIN_DIR/g++ -stdlib=libc++ -nodefaultlibs $STRIP_OPTIONS ${ARCHS} $MIN_OS_VERSION -isysroot $SDKROOT -L"$SOLUTION_DIR/prebuilt/lib/ios/$SDK_NAME" -o "$BUILT_PRODUCTS_DIR/$PRODUCT_NAME.lcext" "$BUILT_PRODUCTS_DIR/$EXECUTABLE_NAME" -Wl,-sectcreate -Wl,__MISC -Wl,__deps -Wl,"$SRCROOT/$PRODUCT_NAME.ios" -Wl,-exported_symbol -Wl,___libinfoptr_$PRODUCT_NAME $STATIC_DEPS

	if [ $? -ne 0 ]; then
		exit $?
	fi
else

	# Only executed if the binaries have a FAT header, and we need an architecture-specific
	# linking
	LCEXT_FILE_LIST=""
	DYLIB_FILE_LIST=""

	# Link architecture-specifically the libraries
	for ARCH in $(echo $ARCHS | tr " " "\n")
	do
	    LCEXT_FILE="$BUILT_PRODUCTS_DIR/$PRODUCT_NAME.lcext_${ARCH}"
	    DYLIB_FILE="$BUILT_PRODUCTS_DIR/$PRODUCT_NAME.dylib_${ARCH}"

	    # arm64 is only from iOS 7.0.0
	    if [ ${ARCH} = "arm64" -o ${ARCH} = "x86_64" ]; then
			MIN_VERSION="7.0.0"
	    else
			MIN_VERSION="6.0.0"
	    fi

		if [ $BUILD_DYLIB -eq 1 ]; then
			OUTPUT=$($BIN_DIR/g++ -stdlib=libc++ -dynamiclib -arch ${ARCH} -miphoneos-version-min=${MIN_VERSION} -isysroot $SDKROOT -L"$SOLUTION_DIR/prebuilt/lib/ios/$SDK_NAME" -o "${DYLIB_FILE}" "$BUILT_PRODUCTS_DIR/$EXECUTABLE_NAME" -dead_strip -Wl,-x $SYMBOLS $DEPS)
			if [ $? -ne 0 ]; then
				echo "Linking ""${DYLIB_FILE}""failed:"
				echo $OUTPUT
				exit $?
			fi
		fi

	    OUTPUT=$($BIN_DIR/g++ -stdlib=libc++ -nodefaultlibs $STRIP_OPTIONS -arch ${ARCH} -miphoneos-version-min=${MIN_VERSION} -isysroot $SDKROOT -L"$SOLUTION_DIR/prebuilt/lib/ios/$SDK_NAME" -o "${LCEXT_FILE}" "$BUILT_PRODUCTS_DIR/$EXECUTABLE_NAME" -Wl,-sectcreate -Wl,__MISC -Wl,__deps -Wl,"$SRCROOT/$PRODUCT_NAME.ios" -Wl,-exported_symbol -Wl,___libinfoptr_$PRODUCT_NAME $STATIC_DEPS)

		if [ $? -ne 0 ]; then
			echo "Linking ""${LCEXT_FILE}""failed:"
			echo $OUTPUT
			exit $?
		fi

	    LCEXT_FILE_LIST+=" ${LCEXT_FILE}"
	    DYLIB_FILE_LIST+=" ${DYLIB_FILE}"
	done

	# Lipo the generated libs
	lipo -create ${LCEXT_FILE_LIST} -output "$BUILT_PRODUCTS_DIR/$PRODUCT_NAME.lcext"

	# Cleanup the lcext_$ARCH files generated
	rm ${LCEXT_FILE_LIST}

	if [ $BUILD_DYLIB -eq 1 ]; then
		lipo -create ${DYLIB_FILE_LIST} -output "$BUILT_PRODUCTS_DIR/$PRODUCT_NAME.dylib"
		rm ${DYLIB_FILE_LIST}
	fi
fi

if [ $BUILD_DYLIB -eq 1 ]; then
	ln -sf "$PRODUCT_NAME.dylib" "$BUILT_PRODUCTS_DIR/$PRODUCT_NAME.ios-extension"
	OUTPUT=$(/usr/bin/codesign -f -s "$CODE_SIGN_IDENTITY" "$BUILT_PRODUCTS_DIR/$PRODUCT_NAME.dylib")
	RESULT=$?
	if [ $RESULT -ne 0 ]; then
		echo "Signing $BUILT_PRODUCTS_DIR/$PRODUCT_NAME.dylib failed:"
		echo $OUTPUT
		exit $RESULT
	fi
else
	ln -sf "$PRODUCT_NAME.lcext" "$BUILT_PRODUCTS_DIR/$PRODUCT_NAME.ios-extension"
fi

if [ "$COPY_PATH" != "" ]; then
	mkdir -p "$COPY_PATH"
	if [ $BUILD_DYLIB -eq 1 ]; then
		cp -f "$BUILT_PRODUCTS_DIR/$PRODUCT_NAME.dylib" "$COPY_PATH/$PRODUCT_NAME.dylib"
	else
		cp -f "$BUILT_PRODUCTS_DIR/$PRODUCT_NAME.lcext" "$COPY_PATH/$PRODUCT_NAME.lcext"
	fi
fi
