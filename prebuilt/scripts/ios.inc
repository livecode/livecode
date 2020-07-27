# List of iOS variants to build
declare -a IOS_SUBPLATFORM
declare -a IOS_XCODE
declare -a IOS_SUBPLATFORM_NAME
declare -a IOS_VERSION
declare -a IOS_ARCHS

function addiOS {
	local SUBPLATFORM=$1
	local XCODE=$2
	local SUBPLATFORM_NAME=$3
	local VERSION=$4
	local ARCHS=$5
	
	echo "Add iOS subplatform ${SUBPLATFORM}"
	
	ELEMENTS=${#IOS_SUBPLATFORM[@]}
	IOS_SUBPLATFORM[$ELEMENTS]=${SUBPLATFORM}
	IOS_XCODE[$ELEMENTS]=${XCODE}
	IOS_SUBPLATFORM_NAME[$ELEMENTS]=${SUBPLATFORM_NAME}
	IOS_VERSION[$ELEMENTS]=${VERSION}
	IOS_ARCHS[$ELEMENTS]=${ARCHS}
	
	export IOS_COUNT=${#IOS_SUBPLATFORM[@]}
}

function queryiOS {
	local SUBPLATFORM=$1
	
	for (( INDEX=0; INDEX<$IOS_COUNT; INDEX++ ))
	do
		if [ "${IOS_SUBPLATFORM[$INDEX]}" == "${SUBPLATFORM}" ] ; then
			export XCODE=${IOS_XCODE[$INDEX]}
			export SUBPLATFORM_NAME=${IOS_SUBPLATFORM_NAME[$INDEX]}
			export VERSION=${IOS_VERSION[$INDEX]}
			export ARCHS=${IOS_ARCHS[$INDEX]}
			
			return 0
		fi
	done
	
	echo "Unknown iOS subplatform: ${SUBPLATFORM}"
	exit 1
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

if [ -z "${XCODE_DEVELOPER}" ] ; then
	ALT_XCODE_DIR=$(xcode-select -print-path)
else
	ALT_XCODE_DIR="${XCODE_DEVELOPER}"
fi

# Detect iOS variants
checkiOS iphonesimulator10.2 8_2_0 iPhoneSimulator 10.2 "i386 x86_64"
checkiOS iphoneos10.2 8_2_0 iPhoneOS 10.2 "armv7 arm64"
checkiOS iphonesimulator11.2 9_2_0 iPhoneSimulator 11.2 "x86_64"
checkiOS iphoneos11.2 9_2_0 iPhoneOS 11.2 "armv7 arm64"
checkiOS iphonesimulator12.1 10_1_0 iPhoneSimulator 12.1 "x86_64"
checkiOS iphoneos12.1 10_1_0 iPhoneOS 12.1 "armv7 arm64"
checkiOS iphonesimulator13.2 11_3_0 iPhoneSimulator 13.2 "x86_64"
checkiOS iphoneos13.2 11_3_0 iPhoneOS 13.2 "armv7 arm64"
checkiOS iphonesimulator13.5 11_5_0 iPhoneSimulator 13.5 "x86_64"
checkiOS iphoneos13.5 11_5_0 iPhoneOS 13.5 "armv7 arm64"
