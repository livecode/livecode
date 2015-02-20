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

# We still want to produce revsecurity.dylib for the simulator
if [ "${SDK_NAME:0:8}" != "iphoneos" -a "${EXECUTABLE_NAME}" = "librevsecurity.a" ]; then
	BUILD_DYLIB=1
else
	BUILD_DYLIB=0
fi

# SN-2015-02-19: [[ Bug 14625 ]] Xcode only create FAT headers from iOS SDK 7.0
EXECUTABLE_INFO=$(otool -fv "$BUILT_PRODUCTS_DIR/$EXECUTABLE_NAME" | grep "Fat headers")
if [ -z "$EXECUTABLE_INFO" ]; then
	if [ $BUILD_DYLIB -eq 1 ]; then
		$BIN_DIR/g++ -stdlib=libc++ -dynamiclib -arch ${ARCHS} -miphoneos-version-min=5.1.1 -isysroot $SDKROOT -L"$SOLUTION_DIR/prebuilt/lib/ios/$SDK_NAME" -o "$BUILT_PRODUCTS_DIR/$PRODUCT_NAME.dylib" "$BUILT_PRODUCTS_DIR/$EXECUTABLE_NAME" -dead_strip -Wl,-x $SYMBOLS $DEPS
	fi
	$BIN_DIR/g++ -stdlib=libc++ -dynamiclib -arch ${ARCHS} -miphoneos-version-min=5.1.1 -isysroot $SDKROOT -L"$SOLUTION_DIR/prebuilt/lib/ios/$SDK_NAME" -o "$BUILT_PRODUCTS_DIR/$PRODUCT_NAME.lcext" "$BUILT_PRODUCTS_DIR/$EXECUTABLE_NAME" -dead_strip -Wl,-x $SYMBOLS $DEPS

	# Success
	exit 0
fi

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
		MIN_VERSION="5.1.1"
    fi

	if [ $BUILD_DYLIB -eq 1 ]; then
		OUTPUT=$($BIN_DIR/g++ -stdlib=libc++ -dynamiclib -arch ${ARCH} -miphoneos-version-min=${MIN_VERSION} -isysroot $SDKROOT -L"$SOLUTION_DIR/prebuilt/lib/ios/$SDK_NAME" -o "${DYLIB_FILE}" "$BUILT_PRODUCTS_DIR/$EXECUTABLE_NAME" -dead_strip -Wl,-x $SYMBOLS $DEPS)
		if [ -n "$OUTPUT" ]; then
			echo "Linking ""${DYLIB_FILE}""failed:"
			echo $OUTPUT
			exit -1
		fi
	fi

    OUTPUT=$($BIN_DIR/g++ -stdlib=libc++ -nodefaultlibs $STRIP_OPTIONS -arch ${ARCH} -miphoneos-version-min=${MIN_VERSION} -isysroot $SDKROOT -L"$SOLUTION_DIR/prebuilt/lib/ios/$SDK_NAME" -o "${LCEXT_FILE}" "$BUILT_PRODUCTS_DIR/$EXECUTABLE_NAME" -Wl,-sectcreate -Wl,__MISC -Wl,__deps -Wl,"$SRCROOT/$PRODUCT_NAME.ios" -Wl,-exported_symbol -Wl,___libinfoptr_$PRODUCT_NAME $STATIC_DEPS)

	if [ -n "$OUTPUT" ]; then
		echo "Linking ""${LCEXT_FILE}""failed:"
		echo $OUTPUT
		exit -1
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
