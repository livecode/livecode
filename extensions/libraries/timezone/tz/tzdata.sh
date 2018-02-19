#!/bin/bash

# Utility script called by the build system to build tzdata.zi

# Arguments 5 and above are the list of zone data files
readonly output_dir=$1
readonly awk_bin=$2
readonly ziguard=$3
readonly zishrink=$4
readonly version_file=$5
shift 5

${awk_bin} -v outfile='main.zi' -f "${ziguard}" $@ > "${output_dir}/main.zi"

version=`sed 1q ${version_file}` && \
	LC_ALL=C ${awk_bin} -v version="$$version" -f "${zishrink}" \
	${output_dir}/main.zi > ${output_dir}/tzdata.zi