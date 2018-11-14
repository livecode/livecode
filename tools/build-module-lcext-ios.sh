#!/bin/bash

set -ex

AUXC_FILE=$1
DEPS_FILE=$2
LCEXT_OUTPUT=$3
MODULE_NAME=$4
IOS_MODULE_OUTPUT=$5

LIBRARIES=""

for LIB in "${@:6}"; do  
	LIBRARIES+="$LIB -force_load $LIB "
done

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

# Only executed if the binaries have a FAT header, and we need an architecture-specific
# linking
LCEXT_FILE_LIST=""

# Link architecture-specifically the libraries
for ARCH in $(echo $ARCHS | tr " " "\n")
do
	LCEXT_FILE="$BUILT_PRODUCTS_DIR/$PRODUCT_NAME.lcext_${ARCH}"


	if [ "$EFFECTIVE_PLATFORM_NAME" = "-iphonesimulator" ]; then
		MIN_OS_VERSION="-mios-simulator-version-min="
	else
		MIN_OS_VERSION="-miphoneos-version-min="
	fi

	# arm64 is only from iOS 7.0.0
	if [ ${ARCH} = "arm64" -o ${ARCH} = "x86_64" ]; then
		MIN_OS_VERSION+="7.0.0"
	else
		MIN_OS_VERSION+="6.0.0"
	fi

	OUTPUT=$($BIN_DIR/g++ -stdlib=libc++ -nodefaultlibs $STRIP_OPTIONS -arch ${ARCH} $MIN_OS_VERSION -isysroot $SDKROOT -o "${LCEXT_FILE}" "$AUXC_FILE" $LIBRARIES -Wl,-sectcreate -Wl,__MISC -Wl,__deps -Wl,"$DEPS_FILE")

	if [ $? -ne 0 ]; then
		echo "Linking ""${LCEXT_FILE}""failed:"
		echo $OUTPUT
		exit $?
	fi

	LCEXT_FILE_LIST+=" ${LCEXT_FILE}"
done

# Lipo the generated libs
lipo -create ${LCEXT_FILE_LIST} -output "$LCEXT_OUTPUT"

# Cleanup the lcext_$ARCH files generated
rm ${LCEXT_FILE_LIST}

# Create ios module file
echo -n "$MODULE_NAME" > "$IOS_MODULE_OUTPUT"
