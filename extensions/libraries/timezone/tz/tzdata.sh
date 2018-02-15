#!/bin/bash

# Utility script called by the build system to build tzdata.zi

set -x

# Arguments 2 and above are the list of zone data files
readonly output_dir=$1

shift 1

awk -v outfile='main.zi' -f ziguard.awk $@ > "${output_dir}/main.zi"

version=`sed 1q version` && \
	LC_ALL=C awk -v version="$$version" -f zishrink.awk \
	${output_dir}/main.zi > ${output_dir}/tzdata.zi