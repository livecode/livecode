DEPS=`cat "$SRCROOT/$PRODUCT_NAME.ios"`
DEPS=${DEPS//library /-l}
DEPS=${DEPS//framework /-framework }

ARCHS="-arch ${ARCHS// / -arch }"
SYMBOLS="-Wl,-exported_symbol -Wl,${SYMBOLS// / -Wl,-exported_symbol -Wl,}"

$PLATFORM_DEVELOPER_BIN_DIR/g++ -dynamiclib $ARCHS -isysroot $SDKROOT -o "$BUILT_PRODUCTS_DIR/$PRODUCT_NAME.dylib" "$BUILT_PRODUCTS_DIR/$EXECUTABLE_NAME" -dead_strip -Wl,-x $SYMBOLS $DEPS

# MW-2011-09-19: Updated to build universal binary version of lcext - by passing
#   the process through g++ we get it all for free!
$PLATFORM_DEVELOPER_BIN_DIR/g++ -nodefaultlibs -Wl,-r -Wl,-x $ARCHS -isysroot $SDKROOT -o "$BUILT_PRODUCTS_DIR/$PRODUCT_NAME.lcext" "$BUILT_PRODUCTS_DIR/$EXECUTABLE_NAME" -Wl,-sectcreate -Wl,__MISC -Wl,__deps -Wl,"$SRCROOT/$PRODUCT_NAME.ios" -Wl,-exported_symbol -Wl,___libinfoptr_$PRODUCT_NAME
#$PLATFORM_DEVELOPER_BIN_DIR/ld -r -x $ARCHS -syslibroot $SDKROOT -o "$BUILT_PRODUCTS_DIR/$PRODUCT_NAME.lcext" "$BUILT_PRODUCTS_DIR/$EXECUTABLE_NAME" -sectcreate __MISC __deps "$SRCROOT/$PRODUCT_NAME.ios" -exported_symbol ___libinfoptr_$PRODUCT_NAME
