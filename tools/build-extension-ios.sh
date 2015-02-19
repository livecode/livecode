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

LCEXT_FILES=""

# Link architecture-specifically the libraries
for ARCH in $(echo $ARCHS | tr " " "\n")
do
    LCEXT_FILE="$BUILT_PRODUCTS_DIR/$PRODUCT_NAME.lcext_${ARCH}"

    # arm64 is only from iOS 7.0.0
    if [ ${ARCH} = "arm64" ]; then
		MIN_VERSION="7.0"
    else
		MIN_VERSION="5.1"
    fi

    $BIN_DIR/g++ -nodefaultlibs $STRIP_OPTIONS -arch ${ARCH} -miphoneos-version-min=${MIN_VERSION} -isysroot $SDKROOT -L"$SOLUTION_DIR/prebuilt/lib/ios/$SDK_NAME" -o "${LCEXT_FILE}" "$BUILT_PRODUCTS_DIR/$EXECUTABLE_NAME" -Wl,-sectcreate -Wl,__MISC -Wl,__deps -Wl,"$SRCROOT/$PRODUCT_NAME.ios" -Wl,-exported_symbol -Wl,___libinfoptr_$PRODUCT_NAME $STATIC_DEPS

    LCEXT_FILES+=" ${LCEXT_FILE}"
done

# Lipo the generated libs
lipo -create ${LCEXT_FILES} -output "$BUILT_PRODUCTS_DIR/$PRODUCT_NAME.lcext"

# Cleanup the lcext_$ARCH files generated
rm ${LCEXT_FILES}
