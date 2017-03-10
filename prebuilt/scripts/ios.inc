# List of iOS variants to build
declare -a IOS_XCODE
declare -a IOS_SUBPLATFORM
declare -a IOS_VERSION
declare -a IOS_ARCHS

function addiOS {
	XCODE=$1
	SUBPLATFORM=$2
	VERSION=$3
	ARCHS=$4
	
	ELEMENTS=${#IOS_XCODE[@]}
	IOS_XCODE[$ELEMENTS]=${XCODE}
	IOS_SUBPLATFORM[$ELEMENTS]=${SUBPLATFORM}
	IOS_VERSION[$ELEMENTS]=${VERSION}
	IOS_ARCHS[$ELEMENTS]=${ARCHS}
	
	export IOS_COUNT=${#IOS_XCODE[@]}
}

# Where to find Xcode installations
XCODES_DIR="${HOME}/Builds/Platforms"

# Detect iOS variants
if [ -e "${XCODES_DIR}/Xcode_6_2_0" ] ; then
	addiOS "${XCODES_DIR}/Xcode_6_2_0/Xcode.app/Contents/Developer" iPhoneSimulator 8.2 "x86_64"
fi
if [ -e "${XCODES_DIR}/Xcode_7_2_0" ] ; then
	addiOS "${XCODES_DIR}/Xcode_7_2_0/Xcode.app/Contents/Developer" iPhoneSimulator 9.2 "x86_64"
	addiOS "${XCODES_DIR}/Xcode_7_2_0/Xcode.app/Contents/Developer" iPhoneOS 9.2 "armv7 arm64"
fi
if [ -e "${XCODES_DIR}/Xcode_8_2_0" ] ; then
	addiOS "${XCODES_DIR}/Xcode_8_2_0/Xcode.app/Contents/Developer" iPhoneSimulator 10.2 "x86_64"
	addiOS "${XCODES_DIR}/Xcode_8_2_0/Xcode.app/Contents/Developer" iPhoneOS 10.2 "armv7 arm64"
fi