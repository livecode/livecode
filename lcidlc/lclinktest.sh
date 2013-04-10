# Get the scripts containing folder
DIR="$( cd "$( dirname "$0" )" && pwd )"

# Compute the input components path
SUFFIX=${SDK_NAME: -3}
SUFFIX="-${SUFFIX//\./_}"
if [ "$EFFECTIVE_PLATFORM_NAME" == "-iphonesimulator" ]; then
	COMPONENTS="$DIR/../components/5_5_3/simulator$SUFFIX"
	SUFFIX=-Simulator$SUFFIX
else
	COMPONENTS="$DIR/../components/5_5_3/device$SUFFIX"
	SUFFIX=-Device$SUFFIX
fi

# Make the executable folder and copy across the fixed files
APP="$BUILT_PRODUCTS_DIR/$EXECUTABLE_FOLDER_PATH"
mkdir -p "$APP"
cp "$COMPONENTS/test" "$APP/."
cp "$COMPONENTS"/*.dylib "$APP/."

# Now copy across the external files
echo $LIVECODE_TEST_EXTERNAL > "$APP/project"
cp "$LIVECODE_TEST_STACK" "$APP/."
cp "$BUILT_PRODUCTS_DIR/$LIVECODE_TEST_EXTERNAL.dylib" "$APP/$LIVECODE_TEST_EXTERNAL.dylib"

