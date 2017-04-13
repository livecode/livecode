#!/bin/sh

set -e

# This script generates a standalone .zip file that contains
# LiveCodeScript tests.

# Preparation
# -----------

# Figure out some paths
top_src_dir=$(cd $(dirname $0)/.. && pwd)
em_bin_dir=${top_src_dir}/emscripten-bin

test_src_dir=${top_src_dir}/tests
em_test_src_dir=${test_src_dir}/_emscripten
test_build_dir=${top_src_dir}/_tests
em_test_build_dir=${test_build_dir}/emscripten
em_stack_dir=${em_test_build_dir}/standalone/boot/standalone

# Get version information
short_ver=$(perl ${top_src_dir}/util/decode_version.pl BUILD_SHORT_VERSION ${top_src_dir}/version)

# Comput boot stack hash
boot_hash=$(head -c1024 ${em_test_src_dir}/__boot.livecodescript | sha1sum -b | cut -f1 -d" ")

# Clean up from previous run and create required directories
rm -rf ${em_test_build_dir}
mkdir -p ${em_test_build_dir}

# Engine
# ------

# Copy the engine into place
cp -a ${em_bin_dir}/standalone-community-${short_ver}.js ${em_test_build_dir}
cp -a ${em_bin_dir}/standalone-community-${short_ver}.html.mem ${em_test_build_dir}
cp -a ${em_bin_dir}/standalone-community-${short_ver}.html ${em_test_build_dir}/tests.html

# Standalone
# ----------
# Construct a basic standalone boot filesystem
cp -r ${top_src_dir}/engine/rsrc/emscripten-standalone-template ${em_test_build_dir}/standalone
# Copy in test suite stacks
mkdir -p ${em_stack_dir}
cp -a ${test_src_dir} ${em_stack_dir}
# Copy in boot stack
cp ${em_test_src_dir}/__boot.livecodescript ${em_test_build_dir}/standalone/boot/standalone/__boot.livecode
# Copy in startup stack and make substitutions
cp ${top_src_dir}/engine/rsrc/emscripten-startup-template.livecodescript ${em_test_build_dir}/standalone/boot/__startup.livecode
sed -i"foo" -e"s,@BOOT_HASH@,${boot_hash}," ${em_test_build_dir}/standalone/boot/__startup.livecode
sed -i"foo" -e"s,@ENGINE_VERSION@,${short_ver}," ${em_test_build_dir}/standalone/boot/__startup.livecode
# Create the standalone.zip file
( cd ${em_test_build_dir}/standalone && zip -0qr ${em_test_build_dir}/standalone.zip * )
