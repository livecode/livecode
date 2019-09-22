#!/bin/bash

source "${BASEDIR}/scripts/platform.inc"
source "${BASEDIR}/scripts/lib_versions.inc"
source "${BASEDIR}/scripts/util.inc"
if [ "${PLATFORM}" == "android" ] ; then
	source "${BASEDIR}/scripts/android.inc"
	configureAndroidToolchain "${ARCH}"
fi

function buildExternal {
	local EXTERNAL_NAME=$1
	local EXTERNAL_CLONE_URL=$2

	local EXTERNAL_VERSION_VAR=${EXTERNAL_NAME}_VERSION
	local EXTERNAL_HASH=${!EXTERNAL_VERSION_VAR}

	local EXTERNAL_BUILDREVISION_VAR=${EXTERNAL_NAME}_BUILDREVISION
	local EXTERNAL_BUILDREVISION=${!EXTERNAL_BUILDREVISION_VAR}

	local CLONE_DIR="${EXTERNAL_NAME}_${EXTERNAL_HASH}_${EXTERNAL_BUILDREVISION}"

	if [ "${ARCH}" == "universal" ] ; then
		local ARCHDIR=
	else
		local ARCHDIR="${ARCH}"
	fi

	local LIBPATH="lib/${PLATFORM}/${ARCHDIR}/${SUBPLATFORM}"

	cd "${BUILDDIR}"

	echo "Cloning ${EXTERNAL_CLONE_URL}"
	git clone "${EXTERNAL_CLONE_URL}" "${CLONE_DIR}"
	if [ $? != 0 ] ; then
		echo "    failed"
		exit $?
	fi

	cd "${CLONE_DIR}"
	echo "Restting to ${EXTERNAL_HASH}"
	git reset --hard "${EXTERNAL_HASH}"
	git submodule sync --recursive
	git submodule update --init --recursive

	mkdir -p "${OUTPUT_DIR}/${LIBPATH}/${EXTERNAL_NAME}"

	case "${PLATFORM}" in
		mac)
			echo "Building ${EXTERNAL_NAME} for Desktop"
			${XCODEBUILD} \
				-project "${EXTERNAL_NAME}.xcodeproj" \
				-configuration Release \
				-sdk ${BUILD_SUBPLATFORM} \
				-target "${EXTERNAL_NAME}-OSX" \
				VALID_ARCHS="${UNIVERSAL_ARCHS}" \
				ARCHS="${UNIVERSAL_ARCHS}" \
				CODE_SIGN_IDENTITY="Developer ID Application: LiveCode Ltd. (KR649NSGHP)" \
				ONLY_ACTIVE_ARCH=NO \
				build \
				-UseModernBuildSystem=NO

			if [ $? != 0 ]; then
				exit $?
			fi
			cp -a "build/Release/${EXTERNAL_NAME}.bundle" "${OUTPUT_DIR}/${LIBPATH}/${EXTERNAL_NAME}"

			echo "Building ${EXTERNAL_NAME} for Server"
			${XCODEBUILD} \
				-project "${EXTERNAL_NAME}.xcodeproj" \
				-configuration Release \
				-sdk ${BUILD_SUBPLATFORM} \
				-target "${EXTERNAL_NAME}-Server" \
				VALID_ARCHS="${UNIVERSAL_ARCHS}" \
				ARCHS="${UNIVERSAL_ARCHS}" \
				CODE_SIGN_IDENTITY="Developer ID Application: LiveCode Ltd. (KR649NSGHP)" \
				ONLY_ACTIVE_ARCH=NO \
				build \
				-UseModernBuildSystem=NO

			if [ $? != 0 ]; then
				exit $?
			fi
			
			cp "build/Release/${EXTERNAL_NAME}.dylib" "${OUTPUT_DIR}/${LIBPATH}/${EXTERNAL_NAME}"
			
			
			;;
		linux)
			if [[ "${ARCH}" == "x86_64" ]]; then
				make "${EXTERNAL_NAME}-x64.so"
				cp "build/${EXTERNAL_NAME}-x64.so" "${OUTPUT_DIR}/${LIBPATH}/${EXTERNAL_NAME}"
			else
				make "${EXTERNAL_NAME}-x86.so"
				cp "build/${EXTERNAL_NAME}-x86.so" "${OUTPUT_DIR}/${LIBPATH}/${EXTERNAL_NAME}"
			fi

			if [ $? != 0 ]; then
				exit $?
			fi
			;;
		android)
			# Local variables
			export SRCROOT="${BUILDDIR}/${CLONE_DIR}/${EXTERNAL_NAME}"
			export DSTROOT=$SRCROOT/_build/android/release
			export RAWROOT=$SRCROOT/_build/android/release/assets

			local JCOUNT=20
			local DEBUGGABLE_FLAG=false
			local DEBUG_FLAG=0

			local NDKBUILD="${ANDROID_NDK}/ndk-build"

			# Build the native code components
			export NDK_PROJECT_PATH=$DSTROOT
			export EXTERNAL_NAME
			echo "Building native code components..."
			$NDKBUILD APP_ABI=$ANDROID_ABI NDK_DEBUG=$DEBUG_FLAG NDK_APP_DEBUGGABLE=$DEBUGGABLE_FLAG NDK_APPLICATION_MK=$SRCROOT/Application.mk -j $JCOUNT -s
			if [ $? != 0 ]; then
				exit $?
			fi

			mkdir -p "${OUTPUT_DIR}/${LIBPATH}/${EXTERNAL_NAME}/Android"
			cp "${DSTROOT}/libs/${ANDROID_ABI}/lib${EXTERNAL_NAME}.so" "${OUTPUT_DIR}/${LIBPATH}/${EXTERNAL_NAME}/Android/External-${ANDROID_ABI}"
			;;
		ios)
			echo "Symlinking in lclink.sh"
			REPO_ROOT=$(git -C "${BUILDDIR}" rev-parse --show-toplevel)
			mkdir -p tools
			cd tools
			ln -sv "${REPO_ROOT}/lcidlc/lclink.sh" "lclink.sh"
			if [ $? != 0 ]; then
				exit $?
			fi
			cd ..

			echo "Building ${EXTERNAL_NAME}"
			${XCODEBUILD} \
				-project "${EXTERNAL_NAME}.xcodeproj" \
				-configuration Release \
				-sdk ${BUILD_SUBPLATFORM} \
				-target "${EXTERNAL_NAME}-iOS" \
				VALID_ARCHS="${UNIVERSAL_ARCHS}" \
				ARCHS="${UNIVERSAL_ARCHS}" \
				CODE_SIGN_IDENTITY="Developer ID Application: LiveCode Ltd. (KR649NSGHP)" \
				LIVECODE_SDKROOT="${BUILDDIR}/${CLONE_DIR}" \
				build \
				-UseModernBuildSystem=NO

			if [ $? != 0 ]; then
				exit $?
			fi
			unzip "binaries/${EXTERNAL_NAME}.lcext" -d "${OUTPUT_DIR}/${LIBPATH}/${EXTERNAL_NAME}"
			;;
	esac
}

buildExternal "mergJSON" "https://github.com/montegoulding/mergjson.git"
buildExternal "mergMarkdown" "https://github.com/montegoulding/mergmarkdown.git"
buildExternal "blur" "https://github.com/montegoulding/blur.git"

