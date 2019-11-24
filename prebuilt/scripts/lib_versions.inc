# This file set the libraries version variables to the version defined in versions/

script_dir=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

function readVersion {
	local LIBNAME=$1
	local LIBNAME_LC=$(tr "[:upper:]" "[:lower:]" <<< $LIBNAME)
	
	local VERSION=$(cat "${script_dir}/../versions/${LIBNAME_LC}")
	eval ${LIBNAME}_VERSION='"${VERSION}"'
	
	if [ -f "${script_dir}/../versions/${LIBNAME_LC}_buildrevision" ] ; then
		local BUILDREVISION=$(cat "${script_dir}/../versions/${LIBNAME_LC}_buildrevision")
		eval ${LIBNAME}_BUILDREVISION='"${BUILDREVISION}"'
	fi
}

readVersion OpenSSL
readVersion ICU
readVersion Curl
readVersion CEF
readVersion CEFChromium

readVersion Thirdparty

# extract hash from start of submodule status
Thirdparty_VERSION=$(expr "$(git submodule status | grep 'thirdparty' )" : '[^0-9a-z]*\([0-9a-z]*\)')

# Libraries used on all platforms
Thirdparty_LIBS_CORE="libffi libpng libz libgif libjpeg libpcre libskia"

# Libraries used only on native (non emscripten) platforms
Thirdparty_LIBS_NATIVE="libmysql libsqlite libxml libxslt libzip"

# Libraries used only on desktop platforms
Thirdparty_LIBS_DESKTOP="libiodbc libpq libcairo"

# Libraries used by emscripten / android for fonts
Thirdparty_LIBS_FONTS="libharfbuzz libfreetype"

Thirdparty_LIBS_mac="$Thirdparty_LIBS_CORE $Thirdparty_LIBS_NATIVE $Thirdparty_LIBS_DESKTOP"
Thirdparty_LIBS_linux="$Thirdparty_LIBS_CORE $Thirdparty_LIBS_NATIVE $Thirdparty_LIBS_DESKTOP"
Thirdparty_LIBS_win32="$Thirdparty_LIBS_CORE $Thirdparty_LIBS_NATIVE $Thirdparty_LIBS_DESKTOP"
Thirdparty_LIBS_ios="$Thirdparty_LIBS_CORE $Thirdparty_LIBS_NATIVE libcairo"
Thirdparty_LIBS_emscripten="$Thirdparty_LIBS_CORE $Thirdparty_LIBS_FONTS"
Thirdparty_LIBS_android="$Thirdparty_LIBS_CORE $Thirdparty_LIBS_NATIVE $Thirdparty_LIBS_FONTS libexpat libcairo"
