# This file set the libraries version variables to the version defined in versions/

script_dir=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

LIBS=( OpenSSL ICU Curl CEF )

for LIB in ${LIBS[@]} ; do
	VERSION=$(cat ${script_dir}/../versions/${LIB,,})
	eval "${LIB}_VERSION=$VERSION"

	# Look for platform-specific version file
	for PLATFORM in ${PLATFORMS[@]} ; do
		if [ -f "${script_dir}/../versions/${LIB,,}_${PLATFORM,,}" ] ; then
			VERSION=$(cat ${script_dir}/../versions/${LIB,,}_${PLATFORM,,})
			eval "${LIB}_${PLATFORM}_VERSION=$VERSION"
		fi
	done
done

