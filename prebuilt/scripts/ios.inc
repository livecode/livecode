# List of iOS variants to build
declare -A IOS_XCODE
declare -A IOS_SUBPLATFORM_NAME
declare -A IOS_VERSION
declare -A IOS_ARCHS

function addiOS {
	SUBPLATFORM=$1
	XCODE=$2
	SUBPLATFORM_NAME=$3
	VERSION=$4
	ARCHS=$5
	
	IOS_XCODE[$SUBPLATFORM]=${XCODE}
	IOS_SUBPLATFORM_NAME[$SUBPLATFORM]=${SUBPLATFORM_NAME}
	IOS_VERSION[$SUBPLATFORM]=${VERSION}
	IOS_ARCHS[$SUBPLATFORM]=${ARCHS}
}

function checkiOS {
	local t_sdk=$1
    local t_xcode_ver=$2
    local t_sdk_name=$3
    local t_sdk_ver=$4
    local t_archs=$5
	 
	 local t_try_xcode="${XCODES_DIR}/Xcode_${t_xcode_ver}"
    if [ -e "${t_try_xcode}" ]; then
	 	addiOS "${t_sdk}" "${t_try_xcode}/Xcode.app/Contents/Developer" "${t_sdk_name}" "${t_sdk_ver}" "${t_archs}"
    elif [ -e "${ALT_XCODE_DIR}/Platforms/${t_sdk_name}.platform/Developer/SDKs/${t_sdk_name}${t_sdk_ver}.sdk" ]; then
	 	addiOS "${t_sdk}" "${ALT_XCODE_DIR}" "${t_sdk_name}" "${t_sdk_ver}" "${t_archs}"
    fi
}

# Where to find Xcode installations
XCODES_DIR="${HOME}/Builds/Platforms"

ALT_XCODE_DIR=$(xcode-select -print-path)

# Detect iOS variants
checkiOS iphonesimulator8.2 6_2_0 iPhoneSimulator 8.2 "i386 x86_64"
checkiOS iphonesimulator9.2 7_2_1 iPhoneSimulator 9.2 "i386 x86_64"
checkiOS iphoneos9.2 7_2_1 iPhoneOS 9.2 "armv7 arm64"
checkiOS iphonesimulator10.2 8_2_0 iPhoneSimulator 10.2 "i386 x86_64"
checkiOS iphoneos10.2 8_2_0 iPhoneOS 10.2 "armv7 arm64"
checkiOS iphonesimulator10.3 8_3_0 iPhoneSimulator 10.3 "i386 x86_64"
checkiOS iphoneos10.3 8_3_0 iPhoneOS 10.3 "armv7 arm64"
