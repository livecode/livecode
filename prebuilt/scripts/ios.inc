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

function checkiOS {
    local t_xcode_ver=$1
    local t_sdk_name=$2
    local t_sdk_ver=$3
    local t_archs=$4
	 
	 local t_try_xcode="${XCODES_DIR}/Xcode_${t_xcode_ver}"
    if [ -e "${t_try_xcode}" ]; then
	 	addiOS "${t_try_xcode}/Xcode.app/Contents/Developer" "${t_sdk_name}" "${t_sdk_ver}" "${t_archs}"
    elif [ -e "${ALT_XCODE_DIR}/Platforms/${t_sdk_name}.platform/Developer/SDKs/${t_sdk_name}${t_sdk_ver}.sdk" ]; then
	 	addiOS "${ALT_XCODE_DIR}" "${t_sdk_name}" "${t_sdk_ver}" "${t_archs}"
    fi
}

# Where to find Xcode installations
XCODES_DIR="${HOME}/Builds/Platforms"

ALT_XCODE_DIR=$(xcode-select -print-path)

# Detect iOS variants
checkiOS 4_6_2 iPhoneSimulator 6.1 i386
checkiOS 5_1_1 iPhoneSimulator 7.1 i386
checkiOS 6_2_0 iPhoneSimulator 8.2 "i386 x86_64"
checkiOS 7_2_1 iPhoneSimulator 9.2 "i386 x86_64"
checkiOS 7_2_1 iPhoneOS 9.2 "armv7 arm64"
checkiOS 8_2_0 iPhoneSimulator 10.2 "i386 x86_64"
checkiOS 8_2_0 iPhoneOS 10.2 "armv7 arm64"
checkiOS 8_3_0 iPhoneSimulator 10.3 "i386 x86_64"
checkiOS 8_3_0 iPhoneOS 10.3 "armv7 arm64"
