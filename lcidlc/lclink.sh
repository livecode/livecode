# Dependencies are calculated from the .ios file that lists which libraries and frameworks to link
# against.
if [ "$LIVECODE_DEP_FILE" == "" ]; then
    LIVECODE_DEP_FILE="$SRCROOT/$PRODUCT_NAME.ios"
fi

read SDK_MAJORVERSION SDK_MINORVERSION <<<${SDK_NAME//[^0-9]/ }
read SDK_PLATFORM <<<${SDK_NAME//[0-9.]/ }

if [ -f "$LIVECODE_DEP_FILE" ]; then

    DEPS=$(cat "$LIVECODE_DEP_FILE")

    STATIC_FRAMEWORKS=$DEPS
    STATIC_FRAMEWORKS="$(sed "/^weak-framework/d" <<< "$STATIC_FRAMEWORKS")"
    STATIC_FRAMEWORKS="$(sed "/^framework/d" <<< "$STATIC_FRAMEWORKS")"
    STATIC_FRAMEWORKS="$(sed "/^library/d" <<< "$STATIC_FRAMEWORKS")"
    STATIC_FRAMEWORKS=${STATIC_FRAMEWORKS//static-framework /-framework }

    # Frameworks may not exist in older sdks so conditionally include
    for MAJORVERSION in 3 4 5 6 7 8 9 10 11; do
        for MINORVERSION in 0 1 2 3 4; do
            if [[ $SDK_MAJORVERSION -lt $MAJORVERSION || ($SDK_MAJORVERSION == $MAJORVERSION && $SDK_MINORVERSION -lt $MINORVERSION) ]]; then
                DEPS="$(sed "/framework-$MAJORVERSION\.$MINORVERSION /d" <<< "$DEPS")"
                DEPS="$(sed "/weak-framework-$MAJORVERSION\.$MINORVERSION /d" <<< "$DEPS")"
            else
                DEPS="$(sed "s/framework-$MAJORVERSION\.$MINORVERSION/framework/g" <<< "$DEPS")"
                DEPS="$(sed "s/weak-framework-$MAJORVERSION\.$MINORVERSION/weak-framework/g" <<< "$DEPS")"
            fi
        done
    done

    DEPS="$(sed "/static-framework /d" <<< "$DEPS")"

    echo -e "$DEPS" > "$LIVECODE_DEP_FILE.tmp"

    DEPS_SECTION="-Wl,-sectcreate -Wl,__MISC -Wl,__deps -Wl,"$LIVECODE_DEP_FILE.tmp""

    DEPS=${DEPS//library /-l}
    DEPS=${DEPS//weak-framework /-weak }
    DEPS=${DEPS//framework /-framework }
    DEPS=${DEPS//-weak /-weak_framework }

    DEPS=$(echo $DEPS)

    # At the moment, lcidlc still includes things that need objective-c, even if 'objc-objects' is not
    # used - thus we force linking to Foundation, dittor for UIKit
    DEPS="$DEPS -framework Foundation -framework UIKit"

fi

# Workaround trailing whitespace and multiple spaces issues
FRAMEWORK_SEARCH_PATHS=$(echo ${FRAMEWORK_SEARCH_PATHS} | xargs)

FRAMEWORK_SEARCH_PATHS="-F${FRAMEWORK_SEARCH_PATHS// / -F}"

LIBRARY_SEARCH_PATHS=$(echo ${LIBRARY_SEARCH_PATHS} | xargs)

LIBRARY_SEARCH_PATHS="-L${LIBRARY_SEARCH_PATHS// / -L}"

# Support using the same script for old externals
if [ "$SYMBOLS" != "_getXtable" ]; then
    # The list of symbols exported by an iOS external is fixed
    SYMBOLS="_MCExternalDescribe _MCExternalInitialize _MCExternalFinalize"
fi

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

# Include custom linker flags
for LDFLAG in $(echo $OTHER_LDFLAGS | tr " " "\n")
do
    OTHER_FLAGS="$OTHER_FLAGS -Wl,$LDFLAG"
done

# We still want to produce dylib for the simulator
if [ "$SDK_PLATFORM" = "iphonesimulator" ]; then
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
        MIN_OS_VERSION="-mios-simulator-version-min=6.0.0"
    else
        MIN_OS_VERSION="-miphoneos-version-min=6.0.0"
    fi

    ARCHS="-arch ${ARCHS// / -arch }"

	# Build the 'dylib' form of the external - this is used by simulator builds, and as
	# a dependency check for device builds - otherwise dsymutil call below may fail
	"$DEVELOPER_BIN_DIR/g++" -stdlib=libc++ -dynamiclib $ARCHS $MIN_OS_VERSION -isysroot "$SDKROOT" $LIBRARY_SEARCH_PATHS $FRAMEWORK_SEARCH_PATHS $STATIC_FRAMEWORKS -o "$BUILT_PRODUCTS_DIR/$PRODUCT_NAME.dylib" "$BUILT_PRODUCTS_DIR/$EXECUTABLE_NAME" $SYMBOL_ARGS $SYMBOLS $DEPS $OTHER_FLAGS
  RESULT=$?
  if [ $RESULT != 0 ]; then
  	exit $RESULT
	fi

	# Only build static library on device builds
	if [ $BUILD_DYLIB -eq 0 ]; then
		"$DEVELOPER_BIN_DIR/g++" -stdlib=libc++ -nodefaultlibs -Wl,-r -Wl,-x $ARCHS $MIN_OS_VERSION -isysroot "$SDKROOT" $LIBRARY_SEARCH_PATHS $FRAMEWORK_SEARCH_PATHS $STATIC_FRAMEWORKS -o "$BUILT_PRODUCTS_DIR/$PRODUCT_NAME.lcext" "$BUILT_PRODUCTS_DIR/$EXECUTABLE_NAME" $DEPS_SECTION $OTHER_FLAGS -Wl,-exported_symbol -Wl,___libinfoptr_$PRODUCT_NAME
    RESULT=$?
    if [ $RESULT != 0 ]; then
    	exit $RESULT
  	fi
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

		if [ $ARCH = "arm64" -o $ARCH = "x86_64" ]; then
			MIN_VERSION="7.0.0"
		else
			MIN_VERSION="6.0.0"
		fi

		# Build the 'dylib' form of the external - this is used by simulator builds, and as
		# a dependency check for device builds.
    "$DEVELOPER_BIN_DIR/g++" -stdlib=libc++ -dynamiclib -arch $ARCH -miphoneos-version-min=${MIN_VERSION} -isysroot "$SDKROOT" $LIBRARY_SEARCH_PATHS $FRAMEWORK_SEARCH_PATHS $STATIC_FRAMEWORKS -o "$DYLIB_FILE" "$BUILT_PRODUCTS_DIR/$EXECUTABLE_NAME" $SYMBOL_ARGS $SYMBOLS $DEPS $OTHER_FLAGS
    RESULT=$?
    if [ $RESULT != 0 ]; then
    	echo "error: linking step of external dylib build failed, probably due to missing framework or library references - check the contents of the $PRODUCT_NAME.ios file"
			exit $RESULT
		fi

		# Build the 'object file' form of the external - this is used by device builds.
		"$DEVELOPER_BIN_DIR/g++" -stdlib=libc++ -nodefaultlibs -Wl,-r -Wl,-x -arch $ARCH  -miphoneos-version-min=${MIN_VERSION} -isysroot "$SDKROOT" $LIBRARY_SEARCH_PATHS $FRAMEWORK_SEARCH_PATHS $STATIC_FRAMEWORKS -o "$LCEXT_FILE" "$BUILT_PRODUCTS_DIR/$EXECUTABLE_NAME" $DEPS_SECTION $OTHER_FLAGS -Wl,-exported_symbol -Wl,___libinfoptr_$PRODUCT_NAME
    RESULT=$?
    if [ $RESULT != 0 ]; then
    	echo "error: linking step of external object build failed"
			exit $RESULT
		fi

		LCEXT_FILE_LIST+=" ${LCEXT_FILE}"
		DYLIB_FILE_LIST+=" ${DYLIB_FILE}"
	done

	# Lipo together the arch-specific binaries and cleanup
	lipo -create ${LCEXT_FILE_LIST} -output "$BUILT_PRODUCTS_DIR/$PRODUCT_NAME.lcext"
	lipo -create ${DYLIB_FILE_LIST} -output "$BUILT_PRODUCTS_DIR/$PRODUCT_NAME.dylib"

	rm -r ${DYLIB_FILE_LIST}
	rm -r ${LCEXT_FILE_LIST}
fi

# Now copy the products into the 'binaries' folder - the dylibs are used for simulator/testing on device
# through XCode; the lcext file is for standalone (device) builds from the IDE.
mkdir -p "$SRCROOT/binaries"

SUFFIX="-${SDK_MAJORVERSION}_${SDK_MINORVERSION}"
if [ "$SDK_PLATFORM" == "iphonesimulator" ]; then
    /usr/bin/codesign --verbose -f -s "$CODE_SIGN_IDENTITY" "$BUILT_PRODUCTS_DIR/$PRODUCT_NAME.dylib"
    RESULT=$?
    if [ $RESULT != 0 ]; then
    	echo "error: code signing"
		exit $RESULT
	 fi
    
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

# Cleanup
if [ -f "$LIVECODE_DEP_FILE.tmp" ]; then
    rm "$LIVECODE_DEP_FILE.tmp"
fi
