#!/bin/bash

set -e

suffix="$1"
shift
inputs="$@"

OBJCOPY="${OBJCOPY:-objcopy}"
STRIP="${STRIP:-strip}"

for input in $inputs ; do
	output="${input}${suffix}"	

	# Extract a copy of the debugging information
	$OBJCOPY --only-keep-debug "$input" "$output" 
	
	# If this file is a dynamic library, don't do a full strip or we'll
	# destroy it (no symbols => can't link to it)
	if [[ "$(objdump -f $input)" =~ "DYNAMIC" ]] ; then
		$STRIP -x --preserve-dates --strip-debug "$input"
	else
		$STRIP -x --preserve-dates --strip-debug --strip-unneeded "$input"
	fi
	
	# Add a hint for the debugger so it can find the debug info
	$OBJCOPY --preserve-dates --remove-section=.gnu_debuglink "$input"
	$OBJCOPY --preserve-dates --add-gnu-debuglink="$output" "$input"
done

