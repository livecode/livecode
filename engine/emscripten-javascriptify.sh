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
whitelist=$3
preamble=$4
shift 4


for lib in $@ ; do
  libs+=\ --js-library\ "${lib}"
done

EMCC=${EMCC:-emcc}
BUILDTYPE=${BUILDTYPE:-Debug}

# Optimisation flags for the Emscripten bitcode-to-javascript step:
#
#	-Os	Optimise for a balance of size and speed
#
if [ "${BUILDTYPE}" = "Release" ] ; then
  optimisation_flags="-Os -g0"
else
  optimisation_flags="-O2 -g3"
fi

${EMCC} ${optimisation_flags} ${CFLAGS} \
	"${input}" \
	-o "${output}" \
	-s EMTERPRETIFY_WHITELIST=@"${whitelist}" \
	-s ASSERTIONS=1 \
	-s EMTERPRETIFY=1 \
	-s EMTERPRETIFY_ASYNC=1 \
	-s ALLOW_MEMORY_GROWTH=1 \
	-s TOTAL_MEMORY=67108864 \
	--pre-js "${preamble}" \
	${libs}

