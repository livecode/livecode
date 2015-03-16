# Dependencies are calculated from the .ios file that lists which libraries and frameworks to link
# against.
if [ "$LIVECODE_DEP_FILE" == "" ]; then
	LIVECODE_DEP_FILE="$SRCROOT/$PRODUCT_NAME.ios"
fi

DEPS=`cat "$LIVECODE_DEP_FILE"`
DEPS=${DEPS//library /-l}
DEPS=${DEPS//weak-framework /-weak }
DEPS=${DEPS//framework /-framework }
DEPS=${DEPS//-weak /-weak_framework }

echo $DEPS

# At the moment, lcidlc still includes things that need objective-c, even if 'objc-objects' is not
# used - thus we force linking to Foundation, dittor for UIKit
DEPS="$DEPS -framework Foundation -framework UIKit"

# The list of symbols exported by an iOS external is fixed
SYMBOLS="_MCExternalDescribe _MCExternalInitialize _MCExternalFinalize"

# Munge the (computed) symbols list into a form suitable for g++
SYMBOLS="-Wl,-exported_symbol -Wl,${SYMBOLS// / -Wl,-exported_symbol -Wl,}"

# SN-2015-02019: [[ Bug 14625 ]] We build and link each arch separately, and lipo them
#  togother once it's done.

# In debug mode we definitely don't want do dead-strip and remove symbols, as then debugging is very
# very hard!
if [ "$BUILD_STYLE" == "Debug" ]; then
	SYMBOL_ARGS=
else
	SYMBOL_ARGS="-dead_strip -Wl,-x"
fi

# We still want to produce dylib for the simulator
if [ "$EFFECTIVE_PLATFORM_NAME" = "-iphonesimulator" ]; then
	BUILD_DYLIB=1
else
	BUILD_DYLIB=0
fi

# SN-2015-02-19: [[ Bug 14625 ]] Xcode only create FAT headers from iOS SDK 7.0
FAT_INFO=$(otool -fv "$BUILT_PRODUCTS_DIR/$EXECUTABLE_NAME" | grep "Fat headers")


if [ -z "$FAT_INFO" -o $BUILD_DYLIB -eq 1 ]; then
	# No arch-specific compilation for simulators.

	# We set the minimum iOS or simulator version
    if [ $BUILD_DYLIB -eq 1 ]; then
        MIN_OS_VERSION="-mios-simulator-version-min=5.1.1"
    else
        MIN_OS_VERSION="-miphoneos-version-min=5.1.1"
    fi

    ARCHS="-arch ${ARCHS// / -arch }"

	if [ $BUILD_DYLIB -eq 1 ]; then
		"$PLATFORM_DEVELOPER_BIN_DIR/g++" -stdlib=libc++ -dynamiclib $ARCHS $MIN_OS_VERSION -isysroot "$SDKROOT" -o "$BUILT_PRODUCTS_DIR/$PRODUCT_NAME.dylib" "$BUILT_PRODUCTS_DIR/$EXECUTABLE_NAME" $SYMBOL_ARGS $SYMBOLS $DEPS
	fi

	if [ $? -ne 0 ]; then
		exit $?
	fi

	"$PLATFORM_DEVELOPER_BIN_DIR/g++" -stdlib=libc++ -nodefaultlibs -Wl,-r -Wl,-x $ARCHS $MIN_OS_VERSION -isysroot "$SDKROOT" -o "$BUILT_PRODUCTS_DIR/$PRODUCT_NAME.lcext" "$BUILT_PRODUCTS_DIR/$EXECUTABLE_NAME" -Wl,-sectcreate -Wl,__MISC -Wl,__deps -Wl,"$LIVECODE_DEP_FILE"
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

		if [ $ARCH = "arm64" -o $ARCH = "x86_64" ]; then
			MIN_VERSION="7.0.0"
		else
			MIN_VERSION="5.1.1"
		fi

		# Build the 'dylib' form of the external - this is used by simulator builds, and as
		# a dependency check for device builds.
		"$PLATFORM_DEVELOPER_BIN_DIR/g++" -stdlib=libc++ -dynamiclib -arch $ARCH -miphoneos-version-min=${MIN_VERSION} -isysroot "$SDKROOT" -o "$DYLIB_FILE" "$BUILT_PRODUCTS_DIR/$EXECUTABLE_NAME" $SYMBOL_ARGS $SYMBOLS $DEPS
		if [ $? != 0 ]; then
			echo "error: linking step of external dylib build failed, probably due to missing framework or library references - check the contents of the $PRODUCT_NAME.ios file"
			exit $?
		fi

		# Build the 'object file' form of the external - this is used by device builds.
		"$PLATFORM_DEVELOPER_BIN_DIR/g++" -stdlib=libc++ -nodefaultlibs -Wl,-r -Wl,-x -arch $ARCH  -miphoneos-version-min=${MIN_VERSION}-isysroot "$SDKROOT" -o "$BUILT_PRODUCTS_DIR/$PRODUCT_NAME.lcext" "$BUILT_PRODUCTS_DIR/$EXECUTABLE_NAME" -Wl,-sectcreate -Wl,__MISC -Wl,__deps -Wl,"$LIVECODE_DEP_FILE"
		if [ $? != 0 ]; then
			echo "error: linking step of external object build failed"
			exit $?
		fi

		LCEXT_FILE_LIST+=" ${LCEXT_FILE}"
		DYLIB_FILE_LIST+=" ${DYLIB_FILE}"
	done

	# Lipo together the arch-specific binaries and cleanup
	lipo -create ${LCEXT_FILE_LIST} -output "$BUILD_PRODUCTS_DIR/$PRODUCT_NAME.lcext"
	lipo -create ${DYLIB_FILE_LIST} -output "$BUILD_PRODUCTS_DIR/$PRODUCT_NAME.dylib"

	rm -r ${DYLIB_FILE_LIST}
	rm -r ${LCEXT_FILE_LIST}
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
