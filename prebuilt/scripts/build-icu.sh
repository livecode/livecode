#!/bin/bash
set -e

source "${BASEDIR}/scripts/platform.inc"
source "${BASEDIR}/scripts/lib_versions.inc"

# Configuration flags
ICU_CONFIG="--disable-shared --enable-static --prefix=/ --with-data-packaging=static --disable-samples --disable-tests --disable-extras"
ICU_CFLAGS="-DU_USING_ICU_NAMESPACE=0 -DUNISTR_FROM_CHAR_EXPLICIT=explicit -DUNISTR_FROM_STRING_EXPLICIT=explicit"

ICU_VERSION_ALT=$(echo "${ICU_VERSION}" | sed 's/\./_/g')

# Grab the source for the library
ICU_TGZ="icu-${ICU_VERSION}.tar.gz"
ICU_SRC="icu-${ICU_VERSION}"
cd "${BUILDDIR}"

# Needed for cross-compiles
case $(uname) in
	Linux*)
		HOST_ICU_DIR="${BUILDDIR}/icu-${ICU_VERSION}-linux-x86_64"
		;;
	Darwin*)
		HOST_ICU_DIR="${BUILDDIR}/icu-${ICU_VERSION}-mac-i386"
		;;
	CYGWIN*)
		HOST_ICU_DIR="${BUILDDIR}/icu-${ICU_VERSION}-win32-i386"
		;;
esac

if [ ! -d "$ICU_SRC" ] ; then
	if [ ! -e "$ICU_TGZ" ] ; then
		echo "Fetching ICU source"
		curl --fail -L http://download.icu-project.org/files/icu4c/${ICU_VERSION}/icu4c-${ICU_VERSION_ALT}-src.tgz -o "${ICU_TGZ}"
		if [ $? != 0 ] ; then
			echo "    failed"
			if [ -e "${ICU_TGZ}" ] ; then 
				rm ${ICU_TGZ} 
			fi
			exit
		fi
	fi
	
	echo "Unpacking ICU source"
	tar -xf "${ICU_TGZ}"
	mv icu "${ICU_SRC}"
fi



function buildICU {
	local SUBPLATFORM_INDEX=$1
	
	for ARCH in ${ARCHS} 
	do
		# Platform-specific options for ICU
		case "${PLATFORM}" in
			mac)
				CONFIG_TYPE="MacOSX"
				if [ "${ARCH}" == "ppc" -o "${ARCH}" == "ppc64" ] ; then
					echo "WARNING: cross-compiled ICU data for PPC does not work"
					CONFIG_TYPE+=" --host=${ARCH}-apple-darwin --with-cross-build=${HOST_ICU_DIR}"
				fi
				;;
			linux)
				CONFIG_TYPE="Linux"
				if [ "${ARCH}" == "x86_64" ] ; then
					CONFIG_TYPE+=" --with-library-bits=64"
				elif [ "${ARCH}" == "armv6-hf" ] ; then
					CONFIG_TYPE+=" --host=arm-linux-gnueabihf --with-cross-build=${HOST_ICU_DIR}"
				else
					CONFIG_TYPE+=" --with-library-bits=32"
				fi
				;;
			android)
				CONFIG_TYPE="Linux --host=arm-linux-androideabi --with-cross-build=${HOST_ICU_DIR} --disable-tools"
				export ANDROID_CFLAGS="-D__STDC_INT64__ -DU_HAVE_NL_LANGINFO_CODESET=0"
				export ANDROID_CXXFLAGS="${ANDROID_CFLAGS}"
				;;
			emscripten)
				CONFIG_FLAGS="--with-cross-build=${HOST_ICU_DIR}"
				;;
			ios)
				CONFIG_TYPE=
				CONFIG_FLAGS="--host=arm-apple-darwin --with-cross-build=${HOST_ICU_DIR} --disable-tools"
				;;
			win32)
				CONFIG_TYPE="Cygwin/MSVC"
				;;
		esac
	
		# Utility for displaying platform name
		if [ ! -z "${SUBPLATFORM_INDEX}" ] ; then
			local SUBPLATFORM="${IOS_SUBPLATFORM[$SUBPLATFORM_INDEX]}${IOS_VERSION[$SUBPLATFORM_INDEX]}"
			local NAME="${PLATFORM}/${SUBPLATFORM}/${ARCH}"
			local PLATFORM_NAME=${SUBPLATFORM}
		else
			local NAME="${PLATFORM}/${ARCH}"
			local PLATFORM_NAME=${PLATFORM}
		fi
		
		ICU_ARCH_SRC="${ICU_SRC}-${PLATFORM_NAME}-${ARCH}"
		ICU_ARCH_LOG="${ICU_ARCH_SRC}.log"
	
		# Copy the source to a target-specific directory
		if [ ! -d "${ICU_ARCH_SRC}" ] ; then
			echo "Creating ICU build directory for ${NAME}"
			mkdir "${ICU_ARCH_SRC}"
		fi
	
		# Get the command used to build a previous copy, if any
		if [ -e "${ICU_ARCH_SRC}/config.cmd" ] ; then
			ICU_ARCH_CURRENT_CONFIG=`cat ${ICU_ARCH_SRC}/config.cmd`
		else
			ICU_ARCH_CURRENT_CONFIG=
		fi
	
		ICU_ARCH_CONFIG="${CONFIG_TYPE} ${ICU_CONFIG} ${CONFIG_FLAGS}"
	
		# Re-configure and re-build, if required
		if [ "${ICU_ARCH_CONFIG}" != "${ICU_ARCH_CURRENT_CONFIG}" ] ; then
			cd "${ICU_ARCH_SRC}"
			echo "Configuring ICU for ${NAME}"
			
			setCCForArch "${ARCH}" "${SUBPLATFORM_INDEX}"
			
			# Method for configuration depends on the platform
			if [ -z "${CONFIG_TYPE}" ] ; then
				${EMCONFIGURE} "../${ICU_SRC}/source/configure" ${ICU_CONFIG} ${CONFIG_FLAGS} > "${ICU_ARCH_LOG}" 2>&1
			else
				"../${ICU_SRC}/source/runConfigureICU" ${CONFIG_TYPE} ${ICU_CONFIG} ${CONFIG_FLAGS} > "${ICU_ARCH_LOG}" 2>&1
			fi
			
			# Disable C++11 support on platforms where we can't guarantee a compatible runtime
			case "${PLATFORM}" in
				android|linux)
					sed -i -e "s/\(^CXXFLAGS.*\)--std=c++0x/\1/" icudefs.mk
					;;
			esac	

			echo "Building ICU for ${NAME}"
			export VERBOSE=1
			${EMMAKE} make clean >> "${ICU_ARCH_LOG}" 2>&1 && \
			    ${EMMAKE} make ${MAKEFLAGS} >> "${ICU_ARCH_LOG}" 2>&1 && \
			    ${EMMAKE} make DESTDIR="${INSTALL_DIR}/${NAME}" install >> "${ICU_ARCH_LOG}" 2>&1
			RESULT=$?
			cd ..
			
			# Save the configuration for this build
			if [ $RESULT == 0 ] ; then
				echo "${ICU_ARCH_CONFIG}" > "${ICU_ARCH_SRC}/config.cmd"
			else
				echo "    failed"
				exit
			fi
		else
			echo "Found existing ICU build for ${NAME}"
		fi
	
		# Generate the minimal data library
		ORIGINAL_DIR=`pwd`
		if [ ! -e "${ICU_ARCH_SRC}/custom-data/icudt${ICU_VERSION_MAJOR}l.dat" ] ; then
			mkdir -p "${ICU_ARCH_SRC}/custom-data"
			cd "${ICU_ARCH_SRC}/custom-data"
			# TODO[Bug 19198] Create custom ICU minimal data for ICU 58
			#curl --fail http://downloads.livecode.com/prebuilts/icudata/minimal/icudt${ICU_VERSION_MAJOR}l.dat -o "icudt${ICU_VERSION_MAJOR}l.dat"
			cp "../data/out/tmp/icudt${ICU_VERSION_MAJOR}l.dat" .
		else
			cd "${ICU_ARCH_SRC}/custom-data"
		fi
		if [ ! -d "extracted" ] ; then
			mkdir -p "extracted"
			"${HOST_ICU_DIR}/bin/icupkg" --list --outlist "icudt${ICU_VERSION_MAJOR}.lst" "icudt${ICU_VERSION_MAJOR}l.dat"
			"${HOST_ICU_DIR}/bin/icupkg" --extract "icudt${ICU_VERSION_MAJOR}.lst" --destdir "./extracted" "icudt${ICU_VERSION_MAJOR}l.dat"
		fi
		if [ ! -d "out-${PLATFORM}-${ARCH}" ] ; then
			mkdir -p "temp"
			mkdir -p "out-${PLATFORM}-${ARCH}"
			"${HOST_ICU_DIR}/bin/pkgdata" --without-assembly --bldopt "../../${ICU_ARCH_SRC}/data/icupkg.inc" --quiet --copyright --sourcedir "./extracted" --destdir "./out-${PLATFORM}-${ARCH}" --entrypoint icudt${ICU_VERSION_MAJOR} --tempdir "./temp" --name "icudt${ICU_VERSION_MAJOR}l" --mode static --revision "${ICU_VERSION}" --libname icudata "icudt${ICU_VERSION_MAJOR}.lst"

			# Copy the data
			rm -r "${INSTALL_DIR}/${NAME}/lib/libicudata.a"
			cp -v "out-${PLATFORM}-${ARCH}/libicudata.a" "${INSTALL_DIR}/${NAME}/lib/libicudata.a"
		fi
		cd "${ORIGINAL_DIR}"

		for L in data i18n io le lx tu uc ; do
			if [ -f "${INSTALL_DIR}/${NAME}/lib/libicu${L}.a" ] ; then
				if [ "${PLATFORM}" == "mac" -o "${PLATFORM}" == "ios" ] ; then
					VAR="ICU${L}_LIBS"
					eval "$VAR+=\"${INSTALL_DIR}/${NAME}/lib/libicu${L}.a \""
				else
					mkdir -p "${OUTPUT_DIR}/lib/${NAME}"
					cp "${INSTALL_DIR}/${NAME}/lib/libicu${L}.a" "${OUTPUT_DIR}/lib/${NAME}/libicu${L}.a"
				fi
			fi
		done
	done

	# Create the universal libraries
	if [ "${PLATFORM}" == "mac" -o "${PLATFORM}" == "ios" ] ; then
		echo "Creating ICU ${PLATFORM_NAME} universal libraries"
		mkdir -p "${OUTPUT_DIR}/lib/${PLATFORM}/${SUBPLATFORM}"
		
		for L in data i18n io le lx tu uc ; do
			VAR="ICU${L}_LIBS"
			eval VALUE=\$$VAR
			if [ ! -z "${VALUE}" ] ; then
				lipo -create $VALUE -output "${OUTPUT_DIR}/lib/${PLATFORM}/${SUBPLATFORM}/libicu${L}.a"
			fi
			eval "$VAR="
		done
	fi
	
	# Copy over the headers, if it has not yet been done
	if [ ! -e "${OUTPUT_DIR}/include/unicode" ] ; then
		echo "Copying ICU headers"
		mkdir -p "${OUTPUT_DIR}/include"
		cp -r "${INSTALL_DIR}/${NAME}/include"/* "${OUTPUT_DIR}/include/"
		
		# Some header massaging is required in order to avoid Win32 link errors
		sed -i "" -e 's/define U_IMPORT __declspec(dllimport)/define U_IMPORT/g' "${OUTPUT_DIR}/include/unicode/platform.h"
		sed -i "" -e 's/define U_EXPORT __declspec(dllexport)/define U_EXPORT/g' "${OUTPUT_DIR}/include/unicode/platform.h"
	fi
}



# If building for iOS, a number of sub-platforms need to be build
if [ "${PLATFORM}" == "ios" ] ; then
	for (( INDEX=0; INDEX<$IOS_COUNT; INDEX++ ))
	do
		setArchs ${INDEX}
		buildICU ${INDEX}
	done
else
	buildICU
fi

