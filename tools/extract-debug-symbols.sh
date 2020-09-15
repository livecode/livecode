#!/bin/bash

set -e

os="$1"
suffix="$2"
shift 2
inputs="$@"

OBJCOPY="${OBJCOPY:-objcopy}"
OBJDUMP="${OBJDUMP:-objdump}"
STRIP="${STRIP:-strip}"

function extract_linux_or_android {

for input in "$@" ; do
	output="${input}${suffix}"

	# The --preserve-dates flag for strip and objcopy only has whole
	# second resolution, so save the timestamps in a separate file
	# instead.
	touch -m -r "$input" "$input.timestamps"

	# Extract a copy of the debugging information
	$OBJCOPY --only-keep-debug "$input" "$output" 
	
	if [ "$os" == "android" -a "$BUILDTYPE" == "Debug" ] ; then
		echo Skipping strip-debug for ${input}
	else
		# Because we export symbols from the engine, only debug symbols
		# should be stripped.
		$STRIP -x --strip-debug "$input"
	fi

	# Add a hint for the debugger so it can find the debug info
	$OBJCOPY --remove-section=.gnu_debuglink "$input"
	$OBJCOPY --add-gnu-debuglink="$output" "$input"

	# Restore the original modification time
	touch -m -r "$input.timestamps" "$input"
	rm "$input.timestamps" 2>&1
done

}

function extract_emscripten {

for input in "$@" ; do
	touch "${input}${suffix}"
done

}

function extract_mac_or_ios {

for input in "$@" ; do
	output="${input}${suffix}"

	# If this is an app bundle, find the executable name
	if [ -f "${input}/Contents/Info.plist" ] ; then
		executable=$(/usr/libexec/PlistBuddy -c "Print CFBundleExecutable" "${input}/Contents/Info.plist")
		if [ $? -eq 0 ] ; then
			real_input="${input}/Contents/MacOS/${executable}"
		else
			echo >&2 Cannot find executable for bundle "${input}" \(no CFBundleExecutable key in plist\)
			exit 1
		fi
	else
		real_input="${input}"
	fi


	# If the OS is iOS and this is a debug build, do nothing
	if [ "$os" == "ios" -a "$BUILDTYPE" == "Debug" ] ; then
		echo Creating empty debug symbols file for ${input}
		touch "${output}"
	else
		# Extract a copy of the debugging information
		echo Extracting debug symbols for ${input}
		dsymutil --out "${output}" "${real_input}"

		# Strip the executable
		$STRIP -x -S "$real_input"
	fi
done

}

case $os in
	linux|android)
		extract_linux_or_android ${inputs}
		;;
	mac|ios)
		extract_mac_or_ios ${inputs}
		;;
	emscripten)
		extract_emscripten ${inputs}
		;;	
	*)
		echo OS "$os" not supported by this script
		exit 1
		;;
esac


