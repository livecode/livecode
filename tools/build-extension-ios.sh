DEPS=`cat "$SRCROOT/$PRODUCT_NAME.ios"`
DEPS=${DEPS//library /-l}
DEPS=${DEPS//framework /-framework }

ARCHS="-arch ${ARCHS// / -arch }"
if [ "$SYMBOLS" != "" ]; then
	SYMBOLS="-Wl,-exported_symbol -Wl,${SYMBOLS// / -Wl,-exported_symbol -Wl,}"
fi

if [ -e "$SYMBOLS_FILE" ]; then
	SYMBOLS+="-Wl,-exported_symbols_list $SYMBOLS_FILE"
fi

if [ "$STATIC_DEPS" != "" ]; then
	DEPS=$STATIC_DEPS
fi
$PLATFORM_DEVELOPER_BIN_DIR/g++ -dynamiclib $ARCHS -isysroot $SDKROOT -L"$SOLUTION_DIR/prebuilt/lib/ios/$SDK_NAME" -o "$BUILT_PRODUCTS_DIR/$PRODUCT_NAME.dylib" "$BUILT_PRODUCTS_DIR/$EXECUTABLE_NAME" -dead_strip -Wl,-x $SYMBOLS $DEPS

# MW-2011-09-19: Updated to build universal binary version of lcext - by passing
#   the process through g++ we get it all for free!
# MW-2013-06-26: [[ CloneAndRun ]] When in 'Debug' mode, don't strip all global symbols.
if [ $CONFIGURATION = "Debug" ]; then
	$PLATFORM_DEVELOPER_BIN_DIR/g++ -nodefaultlibs -Wl,-r $ARCHS -isysroot $SDKROOT -L"$SOLUTION_DIR/prebuilt/lib/ios/$SDK_NAME" -o "$BUILT_PRODUCTS_DIR/$PRODUCT_NAME.lcext" "$BUILT_PRODUCTS_DIR/$EXECUTABLE_NAME" -Wl,-sectcreate -Wl,__MISC -Wl,__deps -Wl,"$SRCROOT/$PRODUCT_NAME.ios" -Wl,-exported_symbol -Wl,___libinfoptr_$PRODUCT_NAME $STATIC_DEPS
else
	$PLATFORM_DEVELOPER_BIN_DIR/g++ -nodefaultlibs -Wl,-r -Wl,-x $ARCHS -isysroot $SDKROOT -L"$SOLUTION_DIR/prebuilt/lib/ios/$SDK_NAME" -o "$BUILT_PRODUCTS_DIR/$PRODUCT_NAME.lcext" "$BUILT_PRODUCTS_DIR/$EXECUTABLE_NAME" -Wl,-sectcreate -Wl,__MISC -Wl,__deps -Wl,"$SRCROOT/$PRODUCT_NAME.ios" -Wl,-exported_symbol -Wl,___libinfoptr_$PRODUCT_NAME $STATIC_DEPS
fi
