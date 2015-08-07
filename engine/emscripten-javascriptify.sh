#!/bin/bash

# This script runs the command that javascript-ifies the bitcode files produced
# by the Emscripten compiler.

set -e

# Parameters:
#	input:		bitcode file to javascriptify
#	output:		the output HTML file
#	exports:	JSON file containing list of exported functions
#	whitelist:	JSON file containing list of emterpreted functions
#	other args:	additional javascript libraries to include
#
input=$1
output=$2
exports=$3
whitelist=$4
preamble=$5
shift 5

for lib in $@ ; do
  libs+=\ --js-library\ "${lib}"
done


emcc -O2 -g ${CFLAGS} \
	"${input}" \
	-o "${output}" \
	-s EXPORTED_FUNCTIONS=@"${exports}" \
	-s EMTERPRETIFY_WHITELIST=@"${whitelist}" \
	-s ASSERTIONS=1 \
	-s EMTERPRETIFY=1 \
	-s EMTERPRETIFY_ASYNC=1 \
	-s ALLOW_MEMORY_GROWTH=1 \
	-s TOTAL_MEMORY=67108864 \
	--preload-file boot \
	--pre-js "${preamble}" \
	${libs}

