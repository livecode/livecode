#!/bin/bash
# Copyright (C) 2015-2016 LiveCode Ltd.
#
# This file is part of LiveCode.
#
# LiveCode is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License v3 as published by the Free
# Software Foundation.
#
# LiveCode is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
#
# You should have received a copy of the GNU General Public License
# along with LiveCode.  If not see <http://www.gnu.org/licenses/>.

# Platforms to triples
platform_to_triple () {
    local platform="$1"
    case "${platform}" in
        linux-*)
            echo "${platform#*-}-linux"
            return 0
            ;;
        mac)
            echo "universal-mac"
            return 0
            ;;
        ios-*)
            echo "universal-ios-sdk_${platform%%-*}"
            return 0
            ;;
        android-armv6)
            echo "armeabi-android"
            return 0
            ;;
        android-armv7)
            echo "armeabi_v7a-android"
            return 0
            ;;
        win-*)
            echo "${platform#*-}-win32"
            return 0
            ;;
        emscripten)
            echo "emscripten"
            return 0
            ;;
    esac
    return 1
}

################################################################
# Parse command-line options
################################################################

num_save_opts=0
outgoing=""
platform=""
target_triple=""

while [[ $# > $num_save_opts ]]; do

  key="$1"

  case "$key" in
    -p|--platform)
        platform="$2"
        shift
        target_triple="$(platform_to_triple ${platform})"
        if [ $? -ne 0 ] ; then
            "ERROR: could not convert platform \"${platform}\" to target triple" >&2
            exit 1
        fi
        outgoing="${outgoing} --target ${target_triple}"
        ;;
    *=*)
        outgoing="${outgoing} ${key}"
        ;;
    *) 
        outgoing="${outgoing} ${key} $2"
        shift
        ;;
  esac

  shift
done

# Run the main configure script
"$(dirname ${BASH_SOURCE[0]})/configure" ${outgoing}
result=$?
if [ $result -ne 0 ] ; then exit $result ; fi

# Set up a compatibility symlink if required
if [ ! -z "${platform}" ] ; then
    rm -r "build-${platform}"
    ln -sv "build-${target_triple}" "build-${platform}"
fi
