#!/bin/bash

# The directory this script lives in
script_dir="$(dirname ${BASH_SOURCE[0]})"

# The symbolicator can only run on OSX
if [[ "$(uname)" != "Darwin" ]] ; then
	echo >&2 "Error: this script must be run under OSX"
	exit 1
fi

# Change to the source tree directory temporarily (so we don't need to worry
# about any special chars in the path to said directory)
pushd >/dev/null "${script_dir}/.."

# Locate a LiveCode engine to run the symbolicator script with
engine_bundle=$(find _build/mac/{Debug,Release}/{LiveCode,Standalone}-Community.app \( -type d \) -print | head -n1)
if [[ -z "${engine_bundle}" ]] ; then
	echo >&2 "Error: could not find LiveCode engine to run symbolicator script"
	exit 2
fi

# Find the name of the executable within the LiveCode bundle 
engine_exe=$(/usr/libexec/PlistBuddy -c "Print CFBundleExecutable" "${engine_bundle}/Contents/Info.plist")
if [[ -z "${engine_exe}" ]] ; then
	echo >&2 "Error: corrupt LiveCode application bundle"
	exit 3
fi

# Return to the original directory
popd >/dev/null

# Run the symbolicator script
"${script_dir}/../${engine_bundle}/Contents/MacOS/${engine_exe}" "${script_dir}/SymbolicatorScript.livecodescript" -ui $@
