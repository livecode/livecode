# Dependencies are calculated from the .ios file that lists which libraries and frameworks to link
# against.
if [ "$LIVECODE_DEP_FILE" == "" ]; then
	LIVECODE_DEP_FILE="$SRCROOT/$PRODUCT_NAME.ios"
fi

if [ -f "$LIVECODE_DEP_FILE" ]; then
    DEPS=`cat "$LIVECODE_DEP_FILE"`
    DEPS=${DEPS//library /-l}
    DEPS=${DEPS//weak-framework /-weak }
    DEPS=${DEPS//framework /-framework }
    DEPS=${DEPS//-weak /-weak_framework }

    echo $DEPS

    # At the moment, lcidlc still includes things that need objective-c, even if 'objc-objects' is not
    # used - thus we force linking to Foundation, dittor for UIKit
    DEPS="$DEPS -framework Foundation -framework UIKit"

    DEPS_SECTION="-Wl,-sectcreate -Wl,__MISC -Wl,__deps -Wl,"$LIVECODE_DEP_FILE""
fi

# The list of symbols exported by an iOS external is fixed
SYMBOLS="_MCExternalDescribe _MCExternalInitialize _MCExternalFinalize"

# Munge the passed in ARCHS environment variable into a form suitable for g++
ARCHS="-arch ${ARCHS// / -arch }"

# Munge the (computed) symbols list into a form suitable for g++
SYMBOLS="-Wl,-exported_symbol -Wl,${SYMBOLS// / -Wl,-exported_symbol -Wl,}"

# In debug mode we definitely don't want do dead-strip and remove symbols, as then debugging is very
# very hard!
if [ "$BUILD_STYLE" == "Debug" ]; then
	SYMBOL_ARGS=
else
	SYMBOL_ARGS="-dead_strip -Wl,-x"
fi

# Build the 'dylib' form of the external - this is used by simulator builds, and as
# a dependency check for device builds.
"$PLATFORM_DEVELOPER_BIN_DIR/g++" -dynamiclib $ARCHS -isysroot "$SDKROOT" -o "$BUILT_PRODUCTS_DIR/$PRODUCT_NAME.dylib" "$BUILT_PRODUCTS_DIR/$EXECUTABLE_NAME" $SYMBOL_ARGS $SYMBOLS $DEPS
if [ $? != 0 ]; then
	echo "error: linking step of external dylib build failed, probably due to missing framework or library references - check the contents of the $PRODUCT_NAME.ios file"
	exit $?
fi

# Build the 'object file' form of the external - this is used by device builds.
"$PLATFORM_DEVELOPER_BIN_DIR/g++" -nodefaultlibs -Wl,-r -Wl,-x $ARCHS -isysroot "$SDKROOT" -o "$BUILT_PRODUCTS_DIR/$PRODUCT_NAME.lcext" "$BUILT_PRODUCTS_DIR/$EXECUTABLE_NAME" $DEPS_SECTION -Wl,-exported_symbol -Wl,___libinfoptr_$PRODUCT_NAME
if [ $? != 0 ]; then
	echo "error: linking step of external object build failed"
	exit $?
fi

# Now copy the products into the 'binaries' folder - the dylibs are used for simulator/testing on device
# through XCode; the lcext file is for standalone (device) builds from the IDE.
mkdir -p "$SRCROOT/binaries"

SUFFIX=${SDK_NAME: -3}
SUFFIX="-${SUFFIX//\./_}"
if [ "$EFFECTIVE_PLATFORM_NAME" == "-iphonesimulator" ]; then
	cp "$BUILT_PRODUCTS_DIR/$PRODUCT_NAME.dylib" "$SRCROOT/binaries/$PRODUCT_NAME-Simulator$SUFFIX.dylib"
	cp "$BUILT_PRODUCTS_DIR/$PRODUCT_NAME.dylib" "$SRCROOT/binaries"
	dsymutil "$SRCROOT/binaries/$PRODUCT_NAME.dylib"

	mkdir -p "$SRCROOT/binaries/iOS"
	cp "$BUILT_PRODUCTS_DIR/$PRODUCT_NAME.dylib" "$SRCROOT/binaries/iOS/External-Simulator$SUFFIX"
	cd binaries
	zip -Rm $PRODUCT_NAME.lcext iOS/*
	cd ..
else
	cp "$BUILT_PRODUCTS_DIR/$PRODUCT_NAME.dylib" "$SRCROOT/binaries/$PRODUCT_NAME-Device$SUFFIX.dylib"
	cp "$BUILT_PRODUCTS_DIR/$PRODUCT_NAME.lcext" "$SRCROOT/binaries/$PRODUCT_NAME-Device$SUFFIX.lcext"
	cp "$BUILT_PRODUCTS_DIR/$PRODUCT_NAME.dylib" "$SRCROOT/binaries"
	dsymutil "$SRCROOT/binaries/$PRODUCT_NAME.dylib"

	mkdir -p "$SRCROOT/binaries/iOS"
	cp "$BUILT_PRODUCTS_DIR/$PRODUCT_NAME.lcext" "$SRCROOT/binaries/iOS/External-Device$SUFFIX"
	cd binaries
	zip -Rm $PRODUCT_NAME.lcext iOS/*
	cd ..
fi
