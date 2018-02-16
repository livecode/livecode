#!/bin/bash

# Utility script called by the build system to build tzdata.zi

set -x

# Arguments 4 and above are the list of zone data files
readonly output_dir=$1
readonly ziguard=$2
readonly zishrink=$3
readonly version_file=$4
shift 4

awk -v outfile='main.zi' -f "${ziguard}" $@ > "${output_dir}/main.zi"

version=`sed 1q ${version_file}` && \
	LC_ALL=C awk -v version="$$version" -f "${zishrink}" \
	${output_dir}/main.zi > ${output_dir}/tzdata.zi