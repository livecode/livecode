#!/bin/bash

source "${BASEDIR}/scripts/platform.inc"
source "${BASEDIR}/scripts/lib_versions.inc"
source "${BASEDIR}/scripts/util.inc"

# Grab the source for the library
if [ "${ARCH}" == "x86" ] ; then
   CEF_DST="cef_binary_${CEF_VERSION}+${CEF_BUILDREVISION}+chromium-${CEFChromium_VERSION}_${PLATFORM}32"
   CEF_SRC="cef_binary_${CEF_VERSION}%2B${CEF_BUILDREVISION}%2Bchromium-${CEFChromium_VERSION}_${PLATFORM}32.tar.bz2"
elif [ "${ARCH}" == "x86_64" ] ; then
   CEF_DST="cef_binary_${CEF_VERSION}+${CEF_BUILDREVISION}+chromium-${CEFChromium_VERSION}_${PLATFORM}64"
   CEF_SRC="cef_binary_${CEF_VERSION}%2B${CEF_BUILDREVISION}%2Bchromium-${CEFChromium_VERSION}_${PLATFORM}64.tar.bz2"
else 
  echo "No binaries available for arch"
fi

CEF_TGZ="${CEF_DST}.tar.bz2"
cd "${BUILDDIR}"

if [ ! -d "$CEF_DST" ] ; then
	if [ ! -e "$CEF_TGZ" ] ; then
		echo "Fetching CEF source"
		fetchUrl "http://opensource.spotify.com/cefbuilds/${CEF_SRC}" "${CEF_TGZ}"
		if [ $? != 0 ] ; then
			echo "downloading http://opensource.spotify.com/cefbuilds/${CEF_SRC} failed"
			if [ -e "${CEF_TGZ}" ] ; then 
				rm ${CEF_TGZ} 
			fi
			exit
		fi
	fi
	
	echo "Unpacking CEF source"
	tar -vjxf "${CEF_TGZ}"
fi
				
# just repackage existing prebuilts and strip unneeded symbols
function buildCEF {
	local PLATFORM=$1
	local ARCH=$2
	
	mkdir -p "${OUTPUT_DIR}/lib/${PLATFORM}/${ARCH}/CEF"
	cp -av "${BUILDDIR}/${CEF_DST}/Release/"* "${OUTPUT_DIR}/lib/${PLATFORM}/${ARCH}/CEF/"
  cp -av "${BUILDDIR}/${CEF_DST}/Resources/"* "${OUTPUT_DIR}/lib/${PLATFORM}/${ARCH}/CEF/"
  strip --strip-unneeded "${OUTPUT_DIR}/lib/${PLATFORM}/${ARCH}/CEF/libcef.so"
}

buildCEF "${PLATFORM}" "${ARCH}"
	
