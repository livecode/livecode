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

for input in $@ ; do
	output="${input}${suffix}"	

	# Extract a copy of the debugging information
	$OBJCOPY --only-keep-debug "$input" "$output" 
	
	# If this file is a dynamic library, don't do a full strip or we'll
	# destroy it (no symbols => can't link to it)
	if [[ "$($OBJDUMP -f $input)" =~ "DYNAMIC" ]] ; then
		$STRIP -x --preserve-dates --strip-debug "$input"
	else
		$STRIP -x --preserve-dates --strip-debug --strip-unneeded "$input"
	fi
	
	# Add a hint for the debugger so it can find the debug info
	$OBJCOPY --preserve-dates --remove-section=.gnu_debuglink "$input"
	$OBJCOPY --preserve-dates --add-gnu-debuglink="$output" "$input"
done

}

function extract_mac_or_ios {

for input in $@ ; do
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
	*)
		echo OS "$os" not supported by this script
		exit 1
		;;
esac


