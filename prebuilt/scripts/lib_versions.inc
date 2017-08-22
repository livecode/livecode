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
